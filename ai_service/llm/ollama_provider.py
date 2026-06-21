"""
Ollama LLM Provider — 本地 LLM 推理

使用 Ollama 的 Python 客户端库（ollama）。
Ollama 是免费、本地运行的 LLM 服务，适合开发阶段。

安装 Ollama：
    https://ollama.com/download
    ollama pull qwen2.5:0.5b    # 轻量模型，适合测试
    ollama pull qwen2.5:7b      # 更强模型
"""

import json
import ollama
from llm.provider import LLMProvider


# NPC 对话的 system prompt 模板
DIALOGUE_SYSTEM_PROMPT = """你是一个游戏中的 NPC 角色。根据你的角色设定，自然地与玩家对话。

你的角色信息：
- 名字：{name}
- 性格：{personality}
- 背景：{background}
- 当前目标：{goal}
- 情绪：{emotion}

玩家信息：
- 状态：{player_state}

对话规则：
1. 保持角色一致性，不要脱离设定
2. 回复要自然、有趣、推动游戏进行
3. 根据你对玩家的好感度调整语气
4. 可以在对话中透露与你的背景故事相关的信息

请以 JSON 格式回复，格式如下：
{{
    "text": "你的对话内容（中文）",
    "options": ["玩家可选回复1", "回复2", "回复3"],
    "mood": "friendly/neutral/hostile",
    "knowledge_gained": ["可能透露的信息点"]
}}

只返回 JSON，不要额外说明。"""


class OllamaProvider(LLMProvider):
    """Ollama 本地 LLM Provider"""

    def __init__(self, model: str = "qwen2.5:0.5b",
                 host: str = "http://localhost:11434"):
        super().__init__(model)
        self.host = host
        self.client = ollama.Client(host=host)
        print(f"[Ollama] Provider ready: model={model}, host={host}")

    def complete(self, messages: list) -> str:
        """标准文本补全"""
        try:
            response = self.client.chat(
                model=self.model,
                messages=messages
            )
            return response["message"]["content"]
        except Exception as e:
            raise RuntimeError(f"Ollama API error: {e}")

    def complete_stream(self, messages: list):
        """流式文本补全"""
        try:
            stream = self.client.chat(
                model=self.model,
                messages=messages,
                stream=True
            )
            for chunk in stream:
                if "message" in chunk and "content" in chunk["message"]:
                    yield chunk["message"]["content"]
        except Exception as e:
            raise RuntimeError(f"Ollama streaming error: {e}")

    def generate_dialogue(self, npc_context: dict, player_state: dict,
                          dialogue_history: list) -> dict:
        """生成 NPC 对话（结构化输出）"""
        # 构建 system prompt
        system_prompt = DIALOGUE_SYSTEM_PROMPT.format(
            name=npc_context.get("name", "未知"),
            personality=npc_context.get("personality", "普通"),
            background=npc_context.get("background", "无"),
            goal=npc_context.get("current_goal", "无特定目标"),
            emotion=npc_context.get("emotional_state", "neutral"),
            player_state=json.dumps(player_state, ensure_ascii=False)
        )

        messages = [{"role": "system", "content": system_prompt}]

        # 添加对话历史
        for entry in dialogue_history[-6:]:  # 最近6条
            role = "assistant" if entry.get("speaker") == "npc" else "user"
            messages.append({"role": role, "content": entry.get("text", "")})

        # 添加玩家当前输入
        messages.append({"role": "user",
                         "content": "（玩家与你交谈）"})

        try:
            raw_response = self.complete(messages)

            # 尝试解析 JSON
            # Ollama 有时会在 JSON 外面加 markdown 代码块
            raw_response = raw_response.strip()
            if raw_response.startswith("```"):
                # 去掉 markdown 代码块标记
                lines = raw_response.split("\n")
                raw_response = "\n".join(lines[1:-1])

            return json.loads(raw_response)

        except json.JSONDecodeError:
            # JSON 解析失败，返回原始文本
            return {
                "text": raw_response if 'raw_response' in dir() else "（NPC 沉默了片刻...）",
                "options": ["继续对话", "询问更多", "离开"],
                "mood": "neutral",
                "knowledge_gained": []
            }
        except Exception as e:
            raise RuntimeError(f"Dialogue generation error: {e}")
