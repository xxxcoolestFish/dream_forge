"""
LLM Provider 抽象基类

所有 LLM 实现（Ollama、Claude、OpenAI 等）须实现此接口。
设计为模型无关，切换模型只需改配置，无需改引擎代码。
"""

from abc import ABC, abstractmethod


class LLMProvider(ABC):
    """LLM Provider 抽象基类"""

    def __init__(self, model: str):
        self.model = model

    @abstractmethod
    def complete(self, messages: list) -> str:
        """
        标准文本补全

        Args:
            messages: [{"role": "system"|"user"|"assistant", "content": "..."}]

        Returns:
            生成的文本
        """
        ...

    @abstractmethod
    def complete_stream(self, messages: list):
        """
        流式文本补全（生成器）

        Yields:
            生成的文本片段
        """
        ...

    @abstractmethod
    def generate_dialogue(self, npc_context: dict, player_state: dict,
                          dialogue_history: list) -> dict:
        """
        生成 NPC 对话（结构化输出）

        Args:
            npc_context: NPC 角色设定
            player_state: 玩家当前状态
            dialogue_history: 最近的对话历史

        Returns:
            {
                "text": "NPC 的对话文本",
                "options": ["玩家可选回复1", "回复2", "回复3"],
                "mood": "friendly" | "neutral" | "hostile",
                "knowledge_gained": ["新信息1", "新信息2"]
            }
        """
        ...

    def count_tokens(self, text: str) -> int:
        """估算 token 数（默认粗略实现）"""
        return len(text) // 4

    def max_context_tokens(self) -> int:
        """最大上下文 token 数"""
        return 4096
