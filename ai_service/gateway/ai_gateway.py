"""
AI 网关 — 统一的 AI 服务入口，管理 LLM Provider。

职责：
    - 根据配置初始化 LLM Provider
    - 提供统一的 chat/dialogue 接口
    - 后续扩展：图像生成、分割等
"""

from llm.provider import LLMProvider
from llm.ollama_provider import OllamaProvider


class AiGateway:
    def __init__(self, provider: str = "ollama", model: str = "qwen2.5:0.5b"):
        self.provider_name = provider
        self.model_name = model

        # 初始化 LLM Provider
        self.llm: LLMProvider = self._create_provider(provider, model)

        print(f"[Gateway] Initialized with {provider}/{model}")

    def _create_provider(self, provider: str, model: str) -> LLMProvider:
        """工厂方法：根据配置创建 LLM Provider"""
        if provider == "ollama":
            return OllamaProvider(model=model)
        else:
            raise ValueError(f"Unknown LLM provider: {provider}")

    def chat(self, messages: list) -> str:
        """标准对话（无游戏上下文）"""
        return self.llm.complete(messages)

    def dialogue(self, npc_context: dict, player_state: dict,
                 dialogue_history: list) -> dict:
        """NPC 对话（包含游戏上下文，返回结构化数据）"""
        return self.llm.generate_dialogue(npc_context, player_state, dialogue_history)
