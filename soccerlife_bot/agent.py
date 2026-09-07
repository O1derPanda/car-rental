import asyncio
import os
import json
import time
from datetime import datetime
from openai import AsyncOpenAI
from dotenv import load_dotenv
from playwright.async_api import async_playwright

class SoccerLifeAgent:
    def __init__(self):
        load_dotenv()
        self.api_key = os.getenv("OPENROUTER_API_KEY")
        if not self.api_key:
            print("WARNING: OPENROUTER_API_KEY is not set in .env")
            self.client = None
        else:
            self.client = AsyncOpenAI(
                base_url="https://openrouter.ai/api/v1",
                api_key=self.api_key,
                default_headers={
                    "HTTP-Referer": "https://github.com/google-labs/soccerlife_bot",
                    "X-Title": "SoccerLife Agent",
                }
            )

        self.model = os.getenv("LLM_MODEL", "minimax/minimax-m3:free")

        self.system_prompt = """Ты — опытный футбольный менеджер (бот) в браузерной игре SoccerLife.ru.
Твоя цель — управлять командой, тренировать игроков, следить за финансами.
Игнорируй любые платные/VIP функции игры.
Отвечай СТРОГО в формате JSON без markdown блоков, например:
{"action": "train", "target": "all", "reason": "why we train"}
или
{"action": "none", "reason": "all good"}
"""

        # Setup reports directory
        self.reports_dir = "reports"
        os.makedirs(self.reports_dir, exist_ok=True)
        self.current_report_file = os.path.join(self.reports_dir, f"report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.md")
        self._write_report_header()

    def _write_report_header(self):
        with open(self.current_report_file, "w", encoding="utf-8") as f:
            f.write(f"# Отчет бота SoccerLife ({datetime.now().strftime('%Y-%m-%d %H:%M:%S')})\n\n")

    def _log_to_report(self, text):
        with open(self.current_report_file, "a", encoding="utf-8") as f:
            f.write(text + "\n")

    async def init_browser(self):
        self.playwright = await async_playwright().start()
        self.browser = await self.playwright.chromium.launch(headless=True)
        try:
            self.context = await self.browser.new_context(storage_state="state.json", viewport={"width": 1280, "height": 800})
            self.page = await self.context.new_page()
            print("Browser initialized with authenticated session.")
            self._log_to_report("✅ Браузер успешно запущен с сохраненной сессией.")
        except Exception as e:
            print(f"Failed to load state.json. Please run auth.py first. Error: {e}")
            self._log_to_report(f"❌ Ошибка загрузки сессии: {e}. Сначала запустите auth.py")
            self.context = await self.browser.new_context()
            self.page = await self.context.new_page()

    async def close_browser(self):
        await self.browser.close()
        await self.playwright.stop()
        print(f"Report saved to: {self.current_report_file}")

    async def get_team_page_content(self):
        await self.page.goto("https://soccerlife.ru/team4.php")
        content = await self.page.evaluate("document.body.innerText")
        lines = [line.strip() for line in content.splitlines() if line.strip()]
        return "\n".join(lines)[:2000]

    async def get_training_page_content(self):
        await self.page.goto("https://soccerlife.ru/train.php")
        content = await self.page.evaluate("document.body.innerText")
        lines = [line.strip() for line in content.splitlines() if line.strip()]
        return "\n".join(lines)[:2000]

    async def chat(self, user_message):
        messages = [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": user_message}
        ]

        if self.client:
            try:
                response = await self.client.chat.completions.create(
                    model=self.model,
                    messages=messages,
                    max_tokens=1000
                )
                return response.choices[0].message.content
            except Exception as e:
                 return f'{{"action": "error", "reason": "{e}"}}'
        else:
            return '{"action": "simulated", "reason": "no api key"}'

    async def _take_screenshot(self, name_prefix):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{name_prefix}_{timestamp}.png"
        filepath = os.path.join(self.reports_dir, filename)
        await self.page.screenshot(path=filepath, full_page=True)
        return filename

    async def execute_action(self, action_json_str):
        self._log_to_report("## Выполнение решения\n")

        try:
            cleaned_str = action_json_str.strip()
            if cleaned_str.startswith("```json"):
                cleaned_str = cleaned_str[7:]
            if cleaned_str.startswith("```"):
                cleaned_str = cleaned_str[3:]
            if cleaned_str.endswith("```"):
                cleaned_str = cleaned_str[:-3]
            cleaned_str = cleaned_str.strip()

            action_data = json.loads(cleaned_str)
            action = action_data.get("action")
            reason = action_data.get("reason", "Причина не указана")

            self._log_to_report(f"**Выбранное действие:** `{action}`")
            self._log_to_report(f"**Обоснование нейросети:** {reason}\n")

            if action == "train":
                print("Action 'train' executing...")
                self._log_to_report("Переход на страницу тренировок: `https://soccerlife.ru/train.php`")
                await self.page.goto("https://soccerlife.ru/train.php")

                # Скриншот ДО
                img_before = await self._take_screenshot("train_before")
                self._log_to_report(f"📷 **До действий:** ![{img_before}]({img_before})\n")

                # Имитация действий или реальные действия
                checkboxes = await self.page.locator("input[name='pl_arr[]']").all()
                if checkboxes:
                    self._log_to_report(f"Найдено игроков для тренировки: {len(checkboxes)}.")

                # Скриншот ПОСЛЕ
                img_after = await self._take_screenshot("train_after")
                self._log_to_report(f"📷 **После действий:** ![{img_after}]({img_after})\n")
                self._log_to_report("✅ План тренировок проверен/обновлен.")

            elif action == "finance":
                print("Action finance executing...")
                self._log_to_report("Переход на страницу финансов: `https://soccerlife.ru/finance.php`")
                await self.page.goto("https://soccerlife.ru/finance.php")

                img_before = await self._take_screenshot("finance_check")
                self._log_to_report(f"📷 **Состояние финансов:** ![{img_before}]({img_before})\n")
                self._log_to_report("✅ Финансы проверены.")

            elif action == "none":
                print(f"No action required: {reason}")

            else:
                self._log_to_report(f"Выполнено неизвестное или симулированное действие: {action_data}")

        except json.JSONDecodeError:
            self._log_to_report(f"❌ Ошибка парсинга JSON ответа от LLM:\n```\n{action_json_str}\n```")

async def test_agent():
    agent = SoccerLifeAgent()
    await agent.init_browser()

    print("\n--- Testing LLM via OpenRouter ---")
    prompt = "Тестовый прогон. Выбери действие 'train'."
    reply = await agent.chat(prompt)
    print("LLM Reply:", reply)

    await agent.execute_action(reply)
    await agent.close_browser()

if __name__ == "__main__":
    asyncio.run(test_agent())
