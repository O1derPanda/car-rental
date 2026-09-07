import requests
from bs4 import BeautifulSoup
import markdownify
import os
import urllib.parse

WIKI_URL = "https://soccerlife.ru/wiki.php"
BASE_URL = "https://soccerlife.ru/"

def get_wiki_links():
    print("Fetching wiki page...")
    response = requests.get(WIKI_URL)
    response.encoding = 'utf-8' # Changed to utf-8 based on curl output
    soup = BeautifulSoup(response.text, 'lxml')

    links = []
    # Find all links that have action=showpost or start with ?action=showpost
    for a in soup.find_all('a', href=True):
        href = a['href']
        if 'action=showpost' in href:
            full_url = urllib.parse.urljoin(WIKI_URL, href)
            links.append((a.text.strip(), full_url))

    return list(dict.fromkeys(links)) # Remove duplicates

def download_and_convert(url):
    try:
        response = requests.get(url)
        response.encoding = 'utf-8'
        soup = BeautifulSoup(response.text, 'lxml')

        # Try to find the main content div. Often it's a specific class or id.
        # Let's extract the main content container.
        content = soup.find('div', class_='info') or soup.find('td', class_='info') or soup.find('div', id='content') or soup.find('body')
        if not content:
            return ""

        md = markdownify.markdownify(str(content), heading_style="ATX")
        return md
    except Exception as e:
        print(f"Error downloading {url}: {e}")
        return ""

def build_knowledge_base():
    links = get_wiki_links()
    print(f"Found {len(links)} links.")

    os.makedirs('knowledge_base', exist_ok=True)

    with open('knowledge_base/index.md', 'w', encoding='utf-8') as index_file:
        index_file.write("# Soccerlife Wiki Knowledge Base\n\n")

        for title, url in links:
            if not title:
                title = "Untitled"
            safe_title = "".join([c if c.isalnum() else "_" for c in title])
            filename = f"{safe_title}.md"
            print(f"Processing: {title} ({url})")

            md_content = download_and_convert(url)

            with open(f'knowledge_base/{filename}', 'w', encoding='utf-8') as f:
                f.write(f"# {title}\n\nOriginal URL: {url}\n\n")
                f.write(md_content)

            index_file.write(f"- [{title}]({filename})\n")

if __name__ == "__main__":
    build_knowledge_base()
