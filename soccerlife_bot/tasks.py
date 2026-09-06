import asyncio
from agent import SoccerLifeAgent

async def run_daily_tasks():
    agent = SoccerLifeAgent()
    await agent.init_browser()

    print("\n--- Running Daily Check ---")

    # 1. Сбор информации о команде
    print("Gathering team info...")
    team_data = await agent.get_team_page_content()

    # 2. Сбор информации о тренировках
    print("Gathering training info...")
    train_data = await agent.get_training_page_content()

    # 3. Принятие решений LLM
    print("Consulting LLM for actions...")
    prompt = f"""
    Данные страницы команды:
    {team_data[:1000]}

    Данные страницы тренировок:
    {train_data[:1000]}

    Исходя из этих данных, какие рутинные задачи ты бы предложил выполнить прямо сейчас?
    (Отвечай строго в JSON формате согласно системному промпту, например {{"action": "train", "target": "all"}}).
    Если действий не требуется, ответь {{"action": "none", "reason": "all good"}}.
    """

    decision_json = await agent.chat(prompt)
    print("LLM Decision:")
    print(decision_json)

    # 4. Реальное выполнение действий на основе JSON
    print("Executing decision...")
    await agent.execute_action(decision_json)

    await agent.close_browser()

if __name__ == "__main__":
    asyncio.run(run_daily_tasks())
