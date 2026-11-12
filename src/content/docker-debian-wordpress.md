##### 本文章适用于拥有Linux云服务器的用户
###### 系统：debian 6.1

##### 第一步自然是更新系统啦!
运行以下代码更新系统<br>
##### `sudo apt update`
##### `sudo apt upgrade`

##### 第二步安装docker
 <a href="https://www.runoob.com/docker/debian-docker-install.html">点击此处前往菜鸟教程查看Docker安装步骤</a>

 ##### * 下面演示debian的安装方法

 ##### 如果你之前安装过 Docker Engine，你需要卸载旧版本，避免冲突：
###### 运行以下代码卸载残留Docker

```bash
for pkg in docker.io docker-doc docker-compose podman-docker containerd runc; do sudo apt-get remove $pkg; done
```

 ##### * 自动化安装
 ###### 官方自动化安装脚本 如果运行有问题请使用卸载代码删除残留文件并使用手动安装

```bash
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
```

 ##### *手动安装
###### 1. 由于第一步已经更新过系统，在确保系统为最新状态后，我们直接开始安装Docker依赖,
这些包允许 apt 使用 HTTPS 协议来访问 Docker 仓库：

`sudo apt install apt-transport-https ca-certificates curl software-properties-common`

###### 2. 使用下面的命令来添加 Docker 官方的 GPG 密钥：

```bash
sudo curl -fsSL https://download.docker.com/linux/debian/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc
```

###### 3.  添加 Docker 官方的 APT 软件源

```bash
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/debian \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
```

###### 4. 更新 APT 软件包缓存

```bash
sudo apt update
```

确保你现在从 Docker 官方仓库安装 Docker 而不是 Debian 默认仓库

```bash
apt-cache policy docker-ce
```

你应该看到它指向 https://download.docker.com/

###### 5. 安装 Docker

```bash
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

###### 6. 验证并将Docker设置为开机自启

```bash
sudo systemctl start docker
sudo systemctl enable docker
```

###### 你可以使用下面命令验证docker是否正常工作

`sudo docker run hello-world`

###### 7. Docker Compose 组件
第 5 步安装的 `docker-compose-plugin` 已经包含 Docker Compose，它可以让我们用一个配置文件定义多个容器。新版使用 `docker compose`（空格）命令调用，注意不再是旧的 `docker-compose`（连字符）写法。

###### 8. 制作配置文件

接下来你需要两个文件,你可以打开记事本编辑后上传到服务器，或者直接用vim编辑，无论用那种方法一定要将两个文件放在同一个文件夹下

下面演示vim编辑
找到你想放文件的位置创建一个文件夹并进入(这个文件夹叫啥都行，你可以直接抄作业)

```bash
mkdir my_wordprice_blog
cd my_wordprice_blog
```

创建一个yml文件，Docker Compose 就是依靠这个来配置镜像的

```bash
vim docker-compose.yml
```

然后按 i 键进入编辑模式 复制下面代码粘贴进去

```yaml
version: '3.3'
services:
   db:
     image: mysql:5.7
     volumes:
       - db_data:/var/lib/mysql
     restart: always
     environment:
       MYSQL_ROOT_PASSWORD: somewordpress
       MYSQL_DATABASE: wordpress
       MYSQL_USER: wordpress
       MYSQL_PASSWORD: wordpress

   wordpress:
     depends_on:
       - db
     volumes:
      - ./wordpress:/var/www/html
      - ./coustom-php.ini:/usr/local/etc/php/conf.d/coustom-php.ini
     image: wordpress:latest
     ports:
       - "80:80"
     restart: always
     environment:
       WORDPRESS_DB_HOST: db:3306
       WORDPRESS_DB_USER: wordpress
       WORDPRESS_DB_PASSWORD: wordpress
       WORDPRESS_DB_NAME: wordpress
volumes:
    db_data: {}
```

完成后按键盘上的ESC键退出编辑模式 并输入 `:wq` 意思是保存并退出

WordPress默认的媒体库上传限制为2M，这限制了许多效果
所以我们还需要一个ini文件来修改配置

在你存放`docker-compose.yml`的文件夹内运行

`vim coustom-php.ini`

同样的,按键盘上的`i`键进入编辑模式
粘贴下面代码将文件上传限制改为1G（参数可变，具体信息可百度，作者自用不太适合所有人）

```ini
file_uploads = On
upload_max_filesize = 1024M
post_max_size = 1024M
max_execution_time = 3000
max_input_time = 3000
memory_limit = 512M
```

同样，完成后按键盘上的ESC键退出编辑模式 并输入 `:wq` 保存并退出

完成上述步骤你的文件夹（my_wordprice_blog）下会有两个文件<br>
`coustom-php.ini`和`docker-compose.yml`

###### 9. 使用 Docker Compose 配置镜像
进入你存放上述文件的地方运行

```bash
sudo docker compose up -d
```

在运行结束后等待一会，你就可以通过你的域名访问WordPress的设置页面了。
