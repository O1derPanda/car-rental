import asyncio
import json
from agent import SoccerLifeAgent
from agent_extensions import get_upcoming_opponents, get_opponent_stats

async def run_daily_tasks():
    agent = SoccerLifeAgent()
    await agent.init_browser()

    agent._log_to_report("\n## Начало сбора данных")
    print("\n--- Running Daily Check ---")

    # 1. Анализ соперников (Предматчевый отчет)
    print("Finding upcoming opponents...")
    opponents = await get_upcoming_opponents(agent)
    agent._log_to_report(f"Найдено ближайших соперников: {len(opponents)}")

    opponents_data = ""
    for opp in opponents:
        print(f"Gathering stats for {opp['name']}...")
        stats = await get_opponent_stats(agent, opp['id'])
        opponents_data += f"**Соперник: {opp['name']}**\n{stats}\n\n"
        agent._log_to_report(f"Собрана статистика по сопернику: {opp['name']}")

    # Получаем краткую инфу о нашей команде для контекста.
    # Делаем это заранее, так как данные нужны и для отчета, и для рутины.
    print("Gathering team info...")
    team_data = await agent.get_team_page_content()

    if opponents_data:
        agent._log_to_report("\n## Предматчевый анализ (Тактика)")
        print("Consulting LLM for tactical advice...")

        tactics_prompt = f"""
        Ты — главный тренер в игре SoccerLife.ru.
        Учти, что механики SoccerLife отличаются от реального футбола (важны скиллы, сыгранность, мораль, усталость, форма).

        Вот состав твоей команды:
        {team_data[:1000]}

        Вот информация о ближайших соперниках (их сила, скиллы, результаты последних 5 матчей):
        {opponents_data}

        Проанализируй соперников и напиши короткий предматчевый отчет-рекомендацию:
        1. Оцени силу соперника по сравнению с нами.
        2. Дай рекомендации по тактике и расстановке строго на основе механик игры SoccerLife.

        Отвечай в простом текстовом формате Markdown (НЕ JSON).
        """

        old_prompt = agent.system_prompt
        agent.system_prompt = "Ты — опытный футбольный менеджер (бот) в браузерной игре SoccerLife.ru. Отвечай в Markdown."

        tactics_report = await agent.chat(tactics_prompt)
        agent.system_prompt = old_prompt

        print("Tactics Report Generated.")
        agent._log_to_report(f"{tactics_report}\n\n---\n")

    # 2. Выполнение рутины (Сбор информации и решение)
    print("Gathering financial/infrastructure info...")
    await agent.page.goto("https://soccerlife.ru/finance.php")
    finance_data = await agent.page.evaluate("document.body.innerText")
    finance_data = "\n".join([line.strip() for line in finance_data.splitlines() if line.strip()])[:1000]
    agent._log_to_report("Собраны данные со страницы финансов.")

    print("Consulting LLM for routine actions...")
    routine_prompt = f"""
    Данные команды:
    {team_data[:1000]}

    Данные финансов:
    {finance_data[:1000]}

    Твоя задача — выбрать, что сделать сейчас (выполни только одно рутинное действие).
    Варианты:
    - "train" (проверить/настроить тренировки)
    - "finance" (если нужно, например, взять кредит или настроить спонсоров)
    - "none" (если всё в порядке и действий не требуется)

    Отвечай СТРОГО в JSON, например: {{"action": "finance", "reason": "need to check sponsors"}}
    """

    agent._log_to_report("\n## Обращение к нейросети (Рутина)")
    decision_json = await agent.chat(routine_prompt)
    print("LLM Decision:", decision_json)
    agent._log_to_report(f"**Сырой ответ от LLM:**\n```json\n{decision_json}\n```\n")

    # 3. Выполнение действий на основе JSON
    print("Executing decision...")
    await agent.execute_action(decision_json)

    await agent.close_browser()

if __name__ == "__main__":
    asyncio.run(run_daily_tasks())
