# 存档系统 (SaveManager)

通过 `narrative` 表进行游戏存档和读档。

## narrative.save(filepath) → bool

将当前游戏状态序列化到 JSON 文件。

```lua
narrative.save("save/slot_01.json")
```

### 序列化内容

| 数据 | 说明 |
|------|------|
| Player Stats | `values`, `maxValues`, `minValues` |
| Player Inventory | `money`, 所有物品实体 |
| Items | 每个物品的完整 `Item` 组件 + `Equipped` 状态 |
| Player Transform | 位置 |
| StoryFlags | 所有剧情标记 |

### JSON 结构示例

```json
{
  "version": 1,
  "player": {
    "transform": { "x": 640, "y": 360, "z": -100 },
    "stats": {
      "values": { "hp": 85, "mp": 45, "level": 3 },
      "maxValues": { "hp": 100, "mp": 50 },
      "minValues": { "hp": 0, "mp": 0 }
    },
    "inventory": {
      "money": 150,
      "items": [
        {
          "itemId": "health_potion",
          "name": "生命药水",
          "consumable": true,
          "useEffects": [{ "stat": "hp", "modify": 30, "clampToMax": true }],
          "statModifiers": {},
          "equipped": false
        }
      ]
    }
  },
  "storyFlags": {
    "chapter1_done": true,
    "met_blacksmith": true
  }
}
```

## narrative.load(filepath) → bool

从 JSON 文件恢复游戏状态。

```lua
if narrative.load("save/slot_01.json") then
    print("读档成功")
else
    print("读档失败（文件不存在或格式错误）")
end
```

### 恢复行为

- **Stats**: 直接覆盖 `values`/`maxValues`/`minValues`
- **Inventory**: 清理旧物品 → 重建所有物品实体 → 恢复金钱
- **Equipped**: 恢复装备状态（不重复应用 statModifiers，避免数值翻倍）
- **StoryFlags**: 完全替换
- **Transform**: 恢复玩家位置

## 注意事项

- 存档使用 `nlohmann/json` 库，文件编码 UTF-8
- 物品在存档中以完整属性存储（不依赖 `item.define` 原型），确保即使原型变更也能正确读档
- 读档会清理并重建玩家背包中的所有物品实体
- 派生属性的值不会单独存储（每次重新计算），存档的是基础属性值
