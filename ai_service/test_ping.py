"""测试 ZeroMQ IPC 通信"""
import zmq
import json

ctx = zmq.Context()
sock = ctx.socket(zmq.REQ)
sock.connect('tcp://127.0.0.1:5555')
sock.send_json({'type': 'ping', 'payload': {}})
resp = sock.recv_json()
print('Response:', json.dumps(resp, indent=2, ensure_ascii=False))
sock.close()
print('✅ IPC ping test passed!')
