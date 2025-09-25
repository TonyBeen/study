#### 生成私钥
`openssl genpkey -algorithm RSA -out private.pem -pkeyopt rsa_keygen_bits:2048`

#### 生成证书
`openssl req -new -x509 -key private.pem -out cert.pem -days 365 -subj "/CN=www.example.com"`
