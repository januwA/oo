
## 安装
```sh
$ git clone git@github.com:januwA/oo.git
$ cd oo
$ cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
$ cmake --build build --config Release
$ ls build/dist
```

## suport https

### windows

[Downlaod OpenSSL-Win64](https://slproweb.com/products/Win32OpenSSL.html)

```sh
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_HTTPS=ON -DOPENSSL_ROOT_DIR="D:\\program\\OpenSSL-Win64" -S . -B build
cmake --build build --config Release
```

### ubuntu
```sh
sudo apt-get install libssl-dev
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_HTTPS=ON -S . -B build
cmake --build build --config Release
```

## options
```
-c  <int>
  set http send count

-u  <http url>                        
  set http url

-m  <head|get|post|delete|put|patch>
  set http method

-h  <key>  <value>
  set http header

-d  <string>
  set http body

-df <name> <string>
  set multipart

-dF <name> <filepath>
  set multipart file

-s  <lua script path>
  set lua script path

-sc  <lua script code string>
  set lua script code string
```

http post
```sh
oo -m post -u http://localhost -d 'id=1&name=oo'
```

http post send file
```sh
oo -m post -u http://localhost -dF files "a.jpg" -dF files "b.jpg"
```

use lua script
```sh
oo -s ./luademo/demo2.lua
```