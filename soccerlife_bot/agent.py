import asyncio
import os
from openai import AsyncOpenAI
from dotenv import load_dotenv
from playwright.async_api import async_playwright
import json

class SoccerLifeAgent:
    def __init__(self):
        load_dotenv()
        self.openai_api_key = os.getenv("OPENAI_API_KEY")
        if not self.openai_api_key:
            print("WARNING: OPENAI_API_KEY is not set in .env")
            # Create a mock client if no key is provided
            self.client = None
        else:
            self.client = AsyncOpenAI(api_key=self.openai_api_key)

        self.system_prompt = """Ты — опытный футбольный менеджер (бот) в браузерной игре SoccerLife.ru.
Твоя цель — управлять командой, тренировать игроков, следить за финансами и развивать инфраструктуру, основываясь на данных, которые ты видишь на сайте.
У тебя есть доступ к базе знаний об игре, если тебе нужны конкретные правила (ты можешь попросить их прочитать).
Всегда отвечай кратко и принимай решения на основе текущего состояния команды.

ВАЖНО: Пользователь НЕ использует подписки/Vip аккаунты. Игнорируй любые платные/VIP функции игры (например, 'Тест драйв Vip', 'виртуальный счет' и другие платные опции).
Отвечай в формате JSON с указанием действия. Формат ответа: {"action": "train", "target": "all"} или {"action": "none", "reason": "all trained"}
"""

    async def init_browser(self):
        self.playwright = await async_playwright().start()
        self.browser = await self.playwright.chromium.launch(headless=True)
        # Load authenticated session
        try:
            self.context = await self.browser.new_context(storage_state="state.json")
            self.page = await self.context.new_page()
            print("Browser initialized with authenticated session.")
        except Exception as e:
            print(f"Failed to load state.json. Please run auth.py first. Error: {e}")
            self.context = await self.browser.new_context()
            self.page = await self.context.new_page()

    async def close_browser(self):
        await self.browser.close()
        await self.playwright.stop()

    async def get_team_page_content(self):
        """Инструмент для LLM: прочитать страницу состава команды."""
        await self.page.goto("https://soccerlife.ru/team4.php")
        # Извлекаем текст
        content = await self.page.evaluate("document.body.innerText")
        # Для удобства LLM можно убрать лишние пробелы и пустые строки
        lines = [line.strip() for line in content.splitlines() if line.strip()]
        return "\n".join(lines)[:4000]

    async def get_training_page_content(self):
        """Инструмент для LLM: прочитать страницу тренировок."""
        await self.page.goto("https://soccerlife.ru/train.php")
        content = await self.page.evaluate("document.body.innerText")
        lines = [line.strip() for line in content.splitlines() if line.strip()]
        return "\n".join(lines)[:4000]

    async def chat(self, user_message):
        messages = [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": user_message}
        ]

        if self.client:
            response = await self.client.chat.completions.create(
                model="gpt-4o-mini",
                messages=messages,
                response_format={ "type": "json_object" }
            )
            return response.choices[0].message.content
        else:
            return '{"action": "simulated", "reason": "no openai key"}'

    async def execute_action(self, action_json_str):
        """Парсит JSON от LLM и выполняет реальное действие в браузере"""
        try:
            action_data = json.loads(action_json_str)
            action = action_data.get("action")

            if action == "train":
                print("Action 'train' executing...")
                await self.page.goto("https://soccerlife.ru/train.php")
                # Кликнуть "Провести тренировку", если кнопка существует (или другая логика)
                # await self.page.click("input[value='Провести тренировку']")
                print("Training completed (mock).")
            elif action == "none":
                print(f"No action required: {action_data.get('reason')}")
            else:
                print(f"Executed simulated action: {action_data}")

        except json.JSONDecodeError:
            print(f"Failed to parse LLM response: {action_json_str}")

async def test_agent():
    agent = SoccerLifeAgent()
    await agent.init_browser()

    # Try reading the team page
    print("\n--- Testing Team Page Read ---")
    team_data = await agent.get_team_page_content()
    print(f"Read {len(team_data)} chars from team page.")

    # Try sending a query to LLM
    print("\n--- Testing LLM ---")
    prompt = f"Вот краткая выжимка страницы 'Моя команда'. {team_data[:1000]} Требуется ли тренировка?"
    reply = await agent.chat(prompt)
    print("LLM Reply:", reply)

    # Выполнение действия
    print("\n--- Executing action ---")
    await agent.execute_action(reply)

    await agent.close_browser()

if __name__ == "__main__":
    asyncio.run(test_agent())
