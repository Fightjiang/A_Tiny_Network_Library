## WebSocket 协议
1. 基于 TCP 传输协议，并复用 HTTP 的握手通道
2. 支持双向通信，实时性强；更好的二进制传输支持；协议控制的数据包头部较小，而 HTTP 每次通信都需要携带完整的头部，从而 WebSocket 可以达到更少的开销控制

## JWT 协议
1. Header;Payload;Signature