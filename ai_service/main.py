"""
AI Game Frame — Python AI 服务主入口

启动 ZeroMQ REP 服务器，等待 C++ 引擎的 AI 请求。

用法：
    python main.py                        # 默认端口 5555
    python main.py --port 5556            # 自定义端口
    python main.py --provider ollama      # 指定 LLM provider
"""

import sys
import signal
import argparse
from gateway.ipc_server import IpcServer


def main():
    parser = argparse.ArgumentParser(description="AI Game Frame Service")
    parser.add_argument("--port", type=int, default=5555,
                        help="ZeroMQ REP 端口 (default: 5555)")
    parser.add_argument("--provider", type=str, default="ollama",
                        help="LLM provider (default: ollama)")
    parser.add_argument("--model", type=str, default="qwen2.5:0.5b",
                        help="LLM model name (default: qwen2.5:0.5b)")
    args = parser.parse_args()

    server = IpcServer(port=args.port, provider=args.provider, model=args.model)

    # 优雅退出
    def shutdown(sig, frame):
        print("\nShutting down AI service...")
        server.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, shutdown)
    signal.signal(signal.SIGTERM, shutdown)

    print(f"=== AI Game Frame Service ===")
    print(f"  Port:     {args.port}")
    print(f"  Provider: {args.provider}")
    print(f"  Model:    {args.model}")
    print(f"  Waiting for engine connection...")
    print()

    server.run()


if __name__ == "__main__":
    main()
