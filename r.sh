#!/bin/bash

# -c  要执行的请求数量
# -m  http的method head|get|post
# -d   post的数据
# -df <name> <string value>    post multipart
# -dF <name> <filepath value>  post multipart 读取filepath文件后上传
# -h <key> <value> 设置请求头
#

# head
# ./build/Debug/oo.exe -m head http://localhost:7777/ping

# get
# ./build/Debug/oo.exe -c 100000 -m get http://localhost:7777/ping

# post json
# ./build/Debug/oo.exe -m post -ct "application/json" -d "{\"name\": 2}" http://localhost:7777/ping

# post form
# ./build/Debug/oo.exe -m post -d "name=a&age=2" http://localhost:7777/ping

# 单文件上传
./build/Debug/oo.exe -c 10 -m post \
  -df name ajanuw \
  -dF file "C:\Users\16418\Pictures\gFXmSqETHQU7zdP.jpg" http://localhost:7777/api/upload/local

# 多文件上传
# ./build/Debug/oo.exe -m post \
#   -dF files "C:\Users\16418\Pictures\gFXmSqETHQU7zdP.jpg" \
#   -dF files "C:\Users\16418\Pictures\KdraGmYFC8Wpsz5.jpg" \
#   http://localhost:7777/api/upload/local_many
