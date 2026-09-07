import bs4

async def get_upcoming_opponents(agent):
    await agent.page.goto("https://soccerlife.ru/team4.php")
    content = await agent.page.content()
    soup = bs4.BeautifulSoup(content, 'html.parser')

    opponents = []

    for div in soup.find_all('div'):
        if 'следующий матч' in div.text.lower():
            parent = div.parent
            links = parent.find_all('a')
            for a in links:
                href = a.get('href', '')
                if 'roster.php?id=' in href:
                    opp_id = href.split('id=')[-1]
                    opp_name = a.text.strip()
                    if opp_name:
                        opponents.append({'id': opp_id, 'name': opp_name})

    unique_opponents = []
    seen = set()
    for opp in opponents:
        if opp['id'] not in seen:
            seen.add(opp['id'])
            if opp['name'] != "Полиспортива":
                unique_opponents.append(opp)


    return unique_opponents[:2]

async def get_opponent_stats(agent, opp_id):
    url = f"https://soccerlife.ru/roster.php?id={opp_id}"
    await agent.page.goto(url)
    content = await agent.page.content()
    soup = bs4.BeautifulSoup(content, 'html.parser')

    stats = []

    # Try to find team power safely
    # Often it's structured like: <div>Сила клуба<br><b>1400</b></div>
    for div in soup.find_all('div'):
        if 'Сила клуба' in div.text:
            text = div.text.strip()
            lines = [l.strip() for l in text.splitlines() if l.strip()]
            # Usually the number follows "Сила клуба"
            if len(lines) > 1 and lines[0] == "Сила клуба" and lines[1].isdigit():
                stats.append(f"Сила клуба: {lines[1]}")
            break

    # Try to find current players to get a sense of squad strength
    players_table = soup.find('table', {'id': 'roster_table'})
    if not players_table:
        for t in soup.find_all('table'):
            if 'Амплуа' in t.text and 'Скилл' in t.text:
                players_table = t
                break

    if players_table:
        skills = []
        rows = players_table.find_all('tr')
        for r in rows:
            cells = r.find_all('td')
            # Look for the skill cell, usually it's one of the last ones with bold text
            for c in cells:
                b = c.find('b')
                if b and b.text.strip().isdigit() and int(b.text.strip()) > 50:
                    skills.append(int(b.text.strip()))
                    break
        if skills:
            avg_skill = sum(skills) / len(skills)
            stats.append(f"Средний скилл игроков (Топ {len(skills)}): {avg_skill:.1f}")
            stats.append(f"Максимальный скилл: {max(skills)}")

    # Get last 5 games
    cal_url = f"https://soccerlife.ru/team4.php?idteam={opp_id}&action=calendar"
    await agent.page.goto(cal_url)
    content = await agent.page.content()
    soup = bs4.BeautifulSoup(content, 'html.parser')

    table = soup.find('table', {'class': 'wide_format'})
    if not table:
        table = soup.find('table')

    if table:
        rows = table.find_all('tr')
        played_matches = []
        for r in rows:
            tds = r.find_all('td')
            if len(tds) >= 4:
                date = tds[0].text.strip()
                comp = tds[1].text.strip()
                opp_name = tds[2].text.strip()
                score = tds[3].text.strip()

                if ':' in score and 'vs' not in score.lower() and len(score) <= 5 and '.' in date:
                    played_matches.append(f"{date} | Турнир: {comp} | Против: {opp_name} | Счет: {score}")

        if played_matches:
            stats.append("\nПоследние сыгранные матчи (до 5 штук):")
            stats.extend(played_matches[-5:])

    # For a team with no players or strength, just add a generic note
    if not stats:
         stats.append("Клуб новый или у него пустой состав (0 игроков). Последних матчей нет.")

    return "\n".join(stats) if stats else "Нет данных о команде."
