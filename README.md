# WebServer
## Get方式
1. 运行src下的a.out 并输入端口号port
2. 在浏览器输入服务器ip和端口号和想要请求的资源
3. 请求报文格式
```
请求行
请求头
空行
请求体（可选）

GET /favicon.ico HTTP/1.1
Host: 192.168.201.128:10000
Connection: keep-alive
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8
Referer: http://192.168.201.128:10000/Login.html
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9

请求体
```
4. 数据流向: 

## POST方式
1. 运行src下的a.out 并输入端口号port
2. 在浏览器输入服务器ip和端口号和想要请求的资源
3. 请求报文格式
```
POST /submit HTTP/1.1
Host: 192.168.201.128:10000
Connection: keep-alive
Content-Length: 45
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36
Content-Type: application/x-www-form-urlencoded
Accept: */*
Origin: http://192.168.201.128:10000
Referer: http://192.168.201.128:10000/Login.html
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9

username=123&email=123123&message=hello+world
```