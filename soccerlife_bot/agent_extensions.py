import bs4
import re

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

                    if not opp_name:
                         # Надежный регексп для извлечения имени соперника из блока следующего матча
                         match = re.search(r'\([^)]+\)\s*(.+?)\s*(Сегодня|Завтра|\d{2}\.\d{2})', parent.text, re.IGNORECASE)
                         if match:
                             opp_name = match.group(1).strip()

                    if opp_name and "Полиспортива" not in opp_name and "Ваш персонаж" not in opp_name:
                        opponents.append({'id': opp_id, 'name': opp_name})

    if not opponents:
        await agent.page.goto("https://soccerlife.ru/action.php?action=gamezone")
        content = await agent.page.content()
        soup = bs4.BeautifulSoup(content, 'html.parser')

        my_team = "Полиспортива"
        rows = soup.find_all('tr')
        for r in rows:
            text = r.text.lower()
            if my_team.lower() in text and 'vs' in text and not ':' in text:
                links = r.find_all('a')
                for a in links:
                    href = a.get('href', '')
                    if 'roster.php?id=' in href and my_team.lower() not in a.text.strip().lower():
                        opponents.append({'id': href.split('id=')[-1], 'name': a.text.strip()})

    unique_opponents = []
    seen = set()
    for opp in opponents:
        if opp['id'] not in seen:
            seen.add(opp['id'])
            unique_opponents.append(opp)

    return unique_opponents[:2]

async def get_opponent_stats(agent, opp_id):
    url = f"https://soccerlife.ru/roster.php?id={opp_id}"
    await agent.page.goto(url)
    content = await agent.page.content()
    soup = bs4.BeautifulSoup(content, 'html.parser')

    stats = []

    for div in soup.find_all('div'):
        if 'Сила клуба' in div.text:
            text = div.text.strip()
            match = re.search(r'Сила клуба\s+(\d+)', text)
            if match:
                stats.append(f"Сила клуба: {match.group(1)}")
            else:
                lines = [l.strip() for l in text.splitlines() if l.strip()]
                if len(lines) > 1 and lines[0] == "Сила клуба" and lines[1].isdigit():
                    stats.append(f"Сила клуба: {lines[1]}")
            break

    players_table = None
    for t in soup.find_all('table'):
        th_texts = [th.text.strip().lower() for th in t.find_all('th')]
        if 'амплуа' in th_texts and 'воз' in th_texts and 'скилл' in th_texts:
            players_table = t
            break

    if players_table:
        skills = []
        rows = players_table.find_all('tr')
        for r in rows[1:]:
            cells = r.find_all('td')
            # Look for the last column that contains a valid skill number
            for c in reversed(cells):
                text_val = c.text.strip()
                if text_val.isdigit():
                    val = int(text_val)
                    if 50 <= val <= 250:
                        skills.append(val)
                        break

        if skills:
            stats.append(f"Количество игроков: {len(skills)}")
            skills.sort(reverse=True)
            top_11 = skills[:11]
            avg_skill = sum(top_11) / len(top_11)
            stats.append(f"Средний скилл топ-11 игроков: {avg_skill:.1f}")
            stats.append(f"Максимальный скилл: {max(skills)}")

    recent_games_found = False
    for div in soup.find_all('div'):
         if 'Календарь игр команды' in div.text:
             parent = div.parent
             rows = parent.find_all('tr')
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
                 recent_games_found = True
             break

    if not recent_games_found:
         cal_url = f"https://soccerlife.ru/team4.php?idteam={opp_id}&action=calendar"
         await agent.page.goto(cal_url)
         content = await agent.page.content()
         cal_soup = bs4.BeautifulSoup(content, 'html.parser')
         table = cal_soup.find('table', {'class': 'wide_format'})
         if not table:
             table = cal_soup.find('table')
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

    if not stats:
         stats.append("Клуб новый или у него пустой состав (0 игроков). Последних матчей нет.")

    return "\n".join(stats) if stats else "Нет данных о команде."
