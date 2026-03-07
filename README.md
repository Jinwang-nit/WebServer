# WebServer
## 使用方法 -- Get
1. 运行src下的a.out 并输入端口号port
2. 在浏览器输入服务器ip和端口号和想要请求的资源
3. 请求报文格式
```
请求行
请求头
空行
请求体（可选）

GET /index.html HTTP/1.1
Host: localhost:10000
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
Accept: text/html,application/xhtml+xml,application/xml
Accept-Language: zh-CN,zh;q=0.9
Connection: keep-alive
Upgrade-Insecure-Requests: 1

请求体
```


