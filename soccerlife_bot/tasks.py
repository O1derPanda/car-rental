import asyncio
from agent import SoccerLifeAgent

async def run_daily_tasks():
    agent = SoccerLifeAgent()
    await agent.init_browser()

    print("\n--- Running Daily Check ---")

    # 1. Сбор информации
    print("Gathering team info...")
    team_data = await agent.get_team_page_content()

    # Можно добавить проверку финансов (base.php / finance.php)
    print("Gathering financial/infrastructure info...")
    await agent.page.goto("https://soccerlife.ru/finance.php")
    finance_data = await agent.page.evaluate("document.body.innerText")
    finance_data = "\n".join([line.strip() for line in finance_data.splitlines() if line.strip()])[:1500]

    print("Consulting LLM for actions based on team and finance data...")
    prompt = f"""
    Данные команды:
    {team_data[:1000]}

    Данные финансов:
    {finance_data[:1000]}

    Твоя задача — выбрать, что сделать сейчас (ты можешь выполнить только одно действие за раз).
    Варианты:
    - "train" (проверить/настроить тренировки)
    - "finance" (если нужно, например, взять кредит или настроить спонсоров)
    - "none" (если всё в порядке и действий не требуется)

    Отвечай СТРОГО в JSON, например: {{"action": "finance", "reason": "need to check sponsors"}}
    """

    decision_json = await agent.chat(prompt)
    print("LLM Decision:")
    print(decision_json)

    # 2. Выполнение действий на основе JSON
    print("Executing decision...")
    await agent.execute_action(decision_json)

    await agent.close_browser()

if __name__ == "__main__":
    asyncio.run(run_daily_tasks())
