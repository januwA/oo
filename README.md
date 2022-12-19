
## 安装
```sh
$ git clone git@github.com:januwA/oo.git
$ cd oo
$ cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
$ cmake --build build --config Release
```

```
-c  <int>                             set http send count
-u  <http url>                        set http url
-m  <head|get|post|delete|put|patch>  set http method
-h  <key>  <value>                    set http header
-d  <string>                          set http body
-df <name> <string>                   set multipart
-dF <name> <filepath>                 set multipart file
-s  <lua script path>                 set lua script path
```

http post
```sh
$ oo -m post -u http://localhost -d 'id=1&name=oo'
```

http post send file
```sh
$ oo -m post -u http://localhost -dF files "a.jpg" -dF files "b.jpg"
```
