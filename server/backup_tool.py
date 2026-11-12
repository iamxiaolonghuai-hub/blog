#!/usr/bin/env python3
"""Blog backup worker. Archives never live beneath the public www directory."""
import datetime as dt
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from zoneinfo import ZoneInfo

ROOT = Path(os.environ.get('BLOG_BACKUP_DIR', '/app/backups'))
UPLOADS = Path(os.environ.get('BLOG_UPLOADS_DIR', '/app/www/uploads'))
SETTINGS = ROOT / 'settings.json'
STATUS = ROOT / 'status.json'
TZ = ZoneInfo('Asia/Shanghai')
ROOT.mkdir(parents=True, exist_ok=True)


def read_json(path, default):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (FileNotFoundError, ValueError):
        return default


def atomic_json(path, value):
    with tempfile.NamedTemporaryFile('w', encoding='utf-8', dir=ROOT, delete=False) as handle:
        json.dump(value, handle, ensure_ascii=False)
        temp = Path(handle.name)
    os.replace(temp, path)


def mysql_env():
    env = os.environ.copy()
    if not env.get('DB_PASSWORD'):
        raise RuntimeError('DB_PASSWORD is not configured')
    env['MYSQL_PWD'] = env['DB_PASSWORD']
    return env


def mysql_args(program):
    return [program, '-h', os.environ.get('DB_HOST', '127.0.0.1'),
            '-P', str(os.environ.get('DB_PORT', '3306')),
            '-u', os.environ.get('DB_USER', 'blog'),
            '--default-character-set=utf8mb4', os.environ.get('DB_NAME', 'blog')]


def make_archive(prefix='blog'):
    stamp = dt.datetime.now(TZ).strftime('%Y%m%d-%H%M%S')
    target = ROOT / f'{prefix}-{stamp}.zip'
    with tempfile.TemporaryDirectory(dir=ROOT) as temporary:
        sql = Path(temporary) / 'blog.sql'
        with sql.open('wb') as output:
            subprocess.run(mysql_args('mariadb-dump')[:-1] +
                           ['--single-transaction', '--routines', '--triggers', '--no-tablespaces',
                            '--add-drop-table', os.environ.get('DB_NAME', 'blog')],
                           env=mysql_env(), stdout=output, check=True)
        if sql.stat().st_size == 0:
            raise RuntimeError('database dump is empty')
        partial = ROOT / (target.name + '.partial')
        try:
            with zipfile.ZipFile(partial, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
                archive.writestr('manifest.json', json.dumps({
                    'format': 'blog-full-backup', 'version': 1,
                    'createdAt': dt.datetime.now(TZ).isoformat(),
                    'database': os.environ.get('DB_NAME', 'blog')}))
                archive.write(sql, 'blog.sql')
                if UPLOADS.exists():
                    for path in UPLOADS.rglob('*'):
                        if path.is_file() and not path.is_symlink():
                            archive.write(path, 'uploads/' + path.relative_to(UPLOADS).as_posix())
            os.replace(partial, target)
        finally:
            partial.unlink(missing_ok=True)
    return target


def remote_copy(archive, settings):
    if not settings.get('remoteEnabled'):
        return
    host, user, path = (settings.get(key, '') for key in ('remoteHost', 'remoteUser', 'remotePath'))
    if not re.fullmatch(r'[A-Za-z0-9.-]+', host) or not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_-]*', user) or not re.fullmatch(r'/[A-Za-z0-9_./-]+', path) or '..' in path.split('/'):
        raise RuntimeError('remote destination is invalid')
    key = ROOT / 'remote_key'
    if not key.is_file():
        raise RuntimeError('remote SSH key is missing')
    port = str(settings.get('remotePort', 22))
    common = ['-i', str(key), '-o', 'BatchMode=yes', '-o', 'StrictHostKeyChecking=accept-new',
              '-o', f'UserKnownHostsFile={ROOT / "known_hosts"}', '-o', 'ConnectTimeout=15']
    subprocess.run(['ssh', *common, '-p', port, f'{user}@{host}', 'mkdir', '-p', path], check=True, timeout=60)
    subprocess.run(['scp', *common, '-P', port, str(archive), f'{user}@{host}:{path}/'], check=True, timeout=600)


def retention(days):
    cutoff = time.time() - max(1, min(int(days), 365)) * 86400
    for path in list(ROOT.glob('blog-*.zip')) + list(ROOT.glob('before-restore-*.zip')):
        if path.stat().st_mtime < cutoff:
            path.unlink()


def validate_archive(archive):
    with zipfile.ZipFile(archive) as bundle:
        names = set(bundle.namelist())
        if 'blog.sql' not in names or 'manifest.json' not in names:
            raise RuntimeError('missing database or manifest')
        manifest = json.loads(bundle.read('manifest.json'))
        if manifest.get('format') != 'blog-full-backup' or manifest.get('version') != 1:
            raise RuntimeError('unsupported backup format')
        if len(names) > 10000 or sum(item.file_size for item in bundle.infolist()) > 2 * 1024**3:
            raise RuntimeError('backup is too large')
        for item in bundle.infolist():
            name = item.filename
            parts = Path(name).parts
            if name not in ('blog.sql', 'manifest.json') and not name.startswith('uploads/'):
                raise RuntimeError('unexpected archive entry')
            if name.startswith('/') or '..' in parts or '\\' in name or item.is_dir() and name in ('blog.sql', 'manifest.json'):
                raise RuntimeError('unsafe archive entry')
        if bundle.testzip() is not None:
            raise RuntimeError('archive checksum failed')


def restore(archive):
    validate_archive(archive)
    safety = make_archive('before-restore')
    with tempfile.TemporaryDirectory(dir=ROOT) as temporary:
        staging = Path(temporary)
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(staging)
        with (staging / 'blog.sql').open('rb') as source:
            subprocess.run(mysql_args('mariadb'), env=mysql_env(), stdin=source, check=True)
        incoming = staging / 'uploads'
        UPLOADS.mkdir(parents=True, exist_ok=True)
        for existing in UPLOADS.iterdir():
            if existing.is_dir() and not existing.is_symlink():
                shutil.rmtree(existing)
            else:
                existing.unlink()
        if incoming.exists():
            for path in incoming.iterdir():
                shutil.move(str(path), UPLOADS / path.name)
    return safety


def run_action(action, archive=None):
    with (ROOT / 'job.lock').open('w') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            return
        atomic_json(STATUS, {'state': 'running', 'action': action, 'startedAt': dt.datetime.now(TZ).isoformat()})
        try:
            settings = read_json(SETTINGS, {})
            if action == 'restore':
                safety = restore(Path(archive))
                message = '恢复完成，恢复前快照：' + safety.name
            else:
                output = make_archive()
                remote_copy(output, settings)
                retention(settings.get('retentionDays', 30))
                message = '备份完成：' + output.name
            atomic_json(STATUS, {'state': 'success', 'action': action,
                                 'message': message, 'finishedAt': dt.datetime.now(TZ).isoformat(),
                                 'lastScheduledDate': settings.get('lastScheduledDate', '')})
        except Exception as exc:
            atomic_json(STATUS, {'state': 'error', 'action': action, 'message': str(exc),
                                 'finishedAt': dt.datetime.now(TZ).isoformat()})
        finally:
            if action == 'restore' and archive:
                Path(archive).unlink(missing_ok=True)


def worker():
    key = ROOT / 'remote_key'
    if not key.exists():
        subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-f', str(key)], check=True)
        key.chmod(0o600)
    while True:
        settings = read_json(SETTINGS, {})
        now = dt.datetime.now(TZ)
        today = now.date().isoformat()
        if settings.get('enabled') and re.fullmatch(r'\d\d:\d\d', settings.get('time', '')):
            hour, minute = map(int, settings['time'].split(':'))
            if (now.hour, now.minute) >= (hour, minute) and settings.get('lastScheduledDate') != today:
                settings['lastScheduledDate'] = today
                atomic_json(SETTINGS, settings)
                run_action('scheduled')
        time.sleep(30)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit('action required')
    if sys.argv[1] == 'worker':
        worker()
    elif sys.argv[1] == 'restore' and len(sys.argv) == 3:
        run_action('restore', sys.argv[2])
    elif sys.argv[1] in ('manual', 'scheduled'):
        run_action(sys.argv[1])
    else:
        sys.exit('invalid action')
