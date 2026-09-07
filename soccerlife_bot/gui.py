import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import asyncio
import threading
import os
import glob
from dotenv import set_key, load_dotenv

# Загружаем существующий .env
load_dotenv()

class SoccerLifeGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("SoccerLife AI Manager")
        self.root.geometry("800x600")

        # Настройка вкладок
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(expand=True, fill='both')

        # Вкладка "Управление"
        self.tab_control = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_control, text='Управление')
        self._setup_control_tab()

        # Вкладка "Настройки"
        self.tab_settings = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_settings, text='Настройки (.env)')
        self._setup_settings_tab()

        # Вкладка "Отчеты"
        self.tab_reports = ttk.Frame(self.notebook)
        self.notebook.add(self.tab_reports, text='Отчеты')
        self._setup_reports_tab()

    def _setup_control_tab(self):
        frame = ttk.Frame(self.tab_control, padding=20)
        frame.pack(fill='both', expand=True)

        ttk.Label(frame, text="SoccerLife AI Manager", font=("Arial", 16, "bold")).pack(pady=10)
        ttk.Label(frame, text="Убедитесь, что логин и пароль указаны в Настройках.", font=("Arial", 10)).pack(pady=5)

        self.btn_auth = ttk.Button(frame, text="1. Авторизация (auth.py)", command=self.run_auth, width=30)
        self.btn_auth.pack(pady=10)

        self.btn_tasks = ttk.Button(frame, text="2. Запустить анализ (tasks.py)", command=self.run_tasks, width=30)
        self.btn_tasks.pack(pady=10)

        ttk.Label(frame, text="Лог выполнения:").pack(anchor='w', pady=(10, 0))
        self.log_area = scrolledtext.ScrolledText(frame, wrap=tk.WORD, height=15)
        self.log_area.pack(fill='both', expand=True)

    def _setup_settings_tab(self):
        frame = ttk.Frame(self.tab_settings, padding=20)
        frame.pack(fill='both', expand=True)

        # Данные для формы
        self.login_var = tk.StringVar(value=os.getenv("SOCCERLIFE_LOGIN", ""))
        self.pass_var = tk.StringVar(value=os.getenv("SOCCERLIFE_PASSWORD", ""))
        self.api_key_var = tk.StringVar(value=os.getenv("OPENROUTER_API_KEY", ""))
        self.model_var = tk.StringVar(value=os.getenv("LLM_MODEL", "google/gemini-2.5-flash"))

        # Создаем поля
        ttk.Label(frame, text="Логин SoccerLife:").grid(row=0, column=0, sticky='w', pady=5)
        ttk.Entry(frame, textvariable=self.login_var, width=40).grid(row=0, column=1, pady=5, padx=5)

        ttk.Label(frame, text="Пароль SoccerLife:").grid(row=1, column=0, sticky='w', pady=5)
        ttk.Entry(frame, textvariable=self.pass_var, show="*", width=40).grid(row=1, column=1, pady=5, padx=5)

        ttk.Label(frame, text="OpenRouter API Key:").grid(row=2, column=0, sticky='w', pady=5)
        ttk.Entry(frame, textvariable=self.api_key_var, show="*", width=40).grid(row=2, column=1, pady=5, padx=5)

        ttk.Label(frame, text="LLM Model:").grid(row=3, column=0, sticky='w', pady=5)
        ttk.Entry(frame, textvariable=self.model_var, width=40).grid(row=3, column=1, pady=5, padx=5)

        ttk.Button(frame, text="Сохранить", command=self.save_settings).grid(row=4, column=1, sticky='e', pady=15)

    def _setup_reports_tab(self):
        frame = ttk.Frame(self.tab_reports, padding=10)
        frame.pack(fill='both', expand=True)

        left_frame = ttk.Frame(frame, width=200)
        left_frame.pack(side='left', fill='y', padx=(0, 10))

        right_frame = ttk.Frame(frame)
        right_frame.pack(side='right', fill='both', expand=True)

        ttk.Button(left_frame, text="Обновить список", command=self.load_reports_list).pack(fill='x', pady=5)

        self.report_listbox = tk.Listbox(left_frame)
        self.report_listbox.pack(fill='both', expand=True)
        self.report_listbox.bind('<<ListboxSelect>>', self.display_report)

        self.report_text = scrolledtext.ScrolledText(right_frame, wrap=tk.WORD)
        self.report_text.pack(fill='both', expand=True)

        self.load_reports_list()

    def log(self, message):
        self.log_area.insert(tk.END, message + "\n")
        self.log_area.see(tk.END)

    def save_settings(self):
        env_path = ".env"
        set_key(env_path, "SOCCERLIFE_LOGIN", self.login_var.get())
        set_key(env_path, "SOCCERLIFE_PASSWORD", self.pass_var.get())
        set_key(env_path, "OPENROUTER_API_KEY", self.api_key_var.get())
        set_key(env_path, "LLM_MODEL", self.model_var.get())
        messagebox.showinfo("Успех", "Настройки сохранены в .env")

    def load_reports_list(self):
        self.report_listbox.delete(0, tk.END)
        if not os.path.exists("reports"):
            return

        reports = sorted(glob.glob("reports/report_*.md"), reverse=True)
        for r in reports:
            # Оставляем только имя файла
            self.report_listbox.insert(tk.END, os.path.basename(r))

    def display_report(self, event):
        selection = self.report_listbox.curselection()
        if not selection:
            return
        filename = self.report_listbox.get(selection[0])
        filepath = os.path.join("reports", filename)

        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()

        self.report_text.delete(1.0, tk.END)
        self.report_text.insert(tk.END, content)

    # Запуск асинхронных задач в отдельном потоке
    def run_auth(self):
        self.btn_auth.config(state='disabled')
        self.log("Запуск авторизации...")
        threading.Thread(target=self._run_script, args=("auth.py", self.btn_auth), daemon=True).start()

    def run_tasks(self):
        self.btn_tasks.config(state='disabled')
        self.log("Запуск анализа (может занять около 30 секунд)...")
        threading.Thread(target=self._run_script, args=("tasks.py", self.btn_tasks), daemon=True).start()

    def _run_script(self, script_name, button):
        import subprocess
        try:
            # Запускаем скрипт и читаем вывод
            process = subprocess.Popen(["python", script_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            for line in iter(process.stdout.readline, ''):
                self.log(line.strip())
            process.stdout.close()
            process.wait()
            self.log(f"--- {script_name} завершен ---")
        except Exception as e:
            self.log(f"Ошибка при запуске {script_name}: {e}")
        finally:
            # Восстанавливаем кнопку в главном потоке
            self.root.after(0, lambda: button.config(state='normal'))
            # Обновляем список отчетов
            self.root.after(0, self.load_reports_list)

if __name__ == "__main__":
    root = tk.Tk()
    app = SoccerLifeGUI(root)
    root.mainloop()
