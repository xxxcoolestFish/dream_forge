"""
ZeroMQ REP 服务器 — 接收 C++ 引擎请求，返回 AI 处理结果。

消息格式（JSON）：
    请求: { "type": "llm_chat" | "ping" | ..., "payload": {...} }
    响应: { "status": "ok" | "error", "data": {...} }

支持的请求类型：
    - ping: 连接测试
    - llm_chat: LLM 对话生成
    - llm_stream: 流式 LLM 对话（计划中）
"""

import zmq
import json
import traceback
from gateway.ai_gateway import AiGateway


class IpcServer:
    def __init__(self, port: int = 5555, provider: str = "ollama",
                 model: str = "qwen2.5:0.5b"):
        self.port = port
        self.address = f"tcp://127.0.0.1:{port}"
        self.running = False

        # 初始化 AI 网关
        self.gateway = AiGateway(provider=provider, model=model)

    def run(self):
        """主循环：接收请求 → 处理 → 返回响应"""
        context = zmq.Context()
        socket = context.socket(zmq.REP)
        socket.bind(self.address)

        self.running = True
        print(f"[IPC] Listening on {self.address}")

        while self.running:
            try:
                # 等待请求
                message = socket.recv_string()
                request = json.loads(message)

                req_type = request.get("type", "")
                payload = request.get("payload", {})

                print(f"[IPC] Received: type={req_type}")

                # 路由到对应处理器
                response = self._handle_request(req_type, payload)

                # 返回响应
                socket.send_string(json.dumps(response, ensure_ascii=False))

            except zmq.ZMQError as e:
                if self.running:
                    print(f"[IPC] ZMQ Error: {e}")
                break
            except json.JSONDecodeError as e:
                print(f"[IPC] JSON Parse Error: {e}")
                error_resp = {"status": "error", "error": f"Invalid JSON: {e}"}
                socket.send_string(json.dumps(error_resp))
            except Exception as e:
                print(f"[IPC] Unexpected Error: {e}")
                traceback.print_exc()
                error_resp = {"status": "error", "error": str(e)}
                try:
                    socket.send_string(json.dumps(error_resp))
                except:
                    pass

        socket.close()
        context.term()
        print("[IPC] Server stopped.")

    def stop(self):
        self.running = False

    def _handle_request(self, req_type: str, payload: dict) -> dict:
        """根据请求类型分发处理"""
        if req_type == "ping":
            return {
                "status": "ok",
                "data": {
                    "message": "pong",
                    "provider": self.gateway.provider_name,
                    "model": self.gateway.model_name
                }
            }

        elif req_type == "llm_chat":
            return self._handle_llm_chat(payload)

        elif req_type == "llm_dialogue":
            return self._handle_llm_dialogue(payload)

        else:
            return {
                "status": "error",
                "error": f"Unknown request type: {req_type}"
            }

    def _handle_llm_chat(self, payload: dict) -> dict:
        """处理标准 LLM 对话请求"""
        messages = payload.get("messages", [])
        if not messages:
            return {"status": "error", "error": "No messages provided"}

        try:
            response_text = self.gateway.chat(messages)
            return {
                "status": "ok",
                "data": {
                    "response": response_text
                }
            }
        except Exception as e:
            return {"status": "error", "error": str(e)}

    def _handle_llm_dialogue(self, payload: dict) -> dict:
        """处理游戏 NPC 对话请求（包含游戏上下文）"""
        npc_context = payload.get("npc", {})
        player_state = payload.get("player", {})
        dialogue_history = payload.get("history", [])

        try:
            response = self.gateway.dialogue(npc_context, player_state, dialogue_history)
            return {
                "status": "ok",
                "data": response
            }
        except Exception as e:
            return {"status": "error", "error": str(e)}
