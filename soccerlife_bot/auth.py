import asyncio
from playwright.async_api import async_playwright
import os
from dotenv import load_dotenv

async def check_login():
    load_dotenv()
    login_str = os.getenv("SOCCERLIFE_LOGIN")
    password_str = os.getenv("SOCCERLIFE_PASSWORD")

    async with async_playwright() as p:
        browser = await p.chromium.launch(headless=True)
        context = await browser.new_context()
        page = await context.new_page()

        print("Go to main page...")
        await page.goto("https://soccerlife.ru/")

        try:
            print("Filling form...")
            await page.fill('input[name="username"]', login_str)
            await page.fill('input[name="password"]', password_str)

            print("Clicking login...")
            # Use Promise.all to wait for navigation and click simultaneously
            async with page.expect_navigation(timeout=10000):
                await page.click('input[name="form_auth"]')

            content_after = await page.content()

            # Check for success indicators
            if "Моя команда" in content_after or "logout" in content_after or "profile.php" in content_after or "Мой клуб" in content_after or "Выход" in content_after:
                 print("SUCCESSFUL_LOGIN")
                 await context.storage_state(path="state.json")
                 print("Session saved to state.json")
            else:
                 print("FAILED_LOGIN")

        except Exception as e:
            print(f"ERROR: {e}")
            # fallback try without navigation wait
            content_after = await page.content()
            if "Моя команда" in content_after or "Выход" in content_after:
                print("SUCCESSFUL_LOGIN_FALLBACK")
                await context.storage_state(path="state.json")
        finally:
            await browser.close()

if __name__ == "__main__":
    asyncio.run(check_login())
