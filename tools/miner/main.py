import os
import json
import argparse
import time
import re
from datetime import datetime
import requests
from bs4 import BeautifulSoup
import feedparser
import schedule 

class TruthMiner:
    def __init__(self):
        self.brain_url = "http://brain:11434/api/generate"
        self.model_name = "mistral"
        self.seen_urls = set()
        
        # [FIX] 시스템 프롬프트 대폭 강화: 엔진이 요구하는 모든 필드를 명시
        self.system_prompt = (
            "You are TopoLogos Sentinel. Analyze the news text and extract structured facts. "
            "Return ONLY a JSON object with the following fields (no markdown, no explanations):\n"
            "{\n"
            "  'summary': 'Concise summary of the event',\n"
            "  'text_clarity_score': (float 0.0-1.0, 1.0 is very clear, 0.0 is jargon salad),\n"
            "  'urgency_keywords_count': (int, count words like 'hurry', 'limited time', 'act now'),\n"
            "  'call_to_action': (bool, true if it tries to sell or force an action),\n"
            "  'input_energy': (float, implied cost/effort, baseline 1.0),\n"
            "  'promised_reward': (float, implied benefit/return, baseline 1.0),\n"
            "  'structure': {'dependency_sources': ['extract', 'all', 'URLs']}\n"
            "}"
        )
        
        self.load_history()
        print("[*] TruthMiner Initialized with Enhanced Vision.")

    def analyze_content(self, text):
        full_prompt = f"{self.system_prompt}\n\nText to analyze:\n{text}"
        print(f"🧠 Asking Brain... (Len: {len(text)})", flush=True)

        payload = {
            "model": self.model_name,
            "prompt": full_prompt,
            "stream": False,
            "format": "json" 
        }
        
        try:
            response = requests.post(self.brain_url, json=payload, timeout=600)
            response.raise_for_status()
            result = response.json()
            return result['response']
        except Exception as e:
            print(f"[!] Brain Error: {e}", flush=True)
            return None

    def load_history(self):
        inbox_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
        if os.path.exists(inbox_dir):
            for f in os.listdir(inbox_dir):
                if f.endswith(".topo.json"):
                    self.seen_urls.add(f.replace(".topo.json", ""))

    def fetch_url(self, url: str) -> str:
        # (이전과 동일)
        try:
            headers = {'User-Agent': 'TopoLogos/3.0'}
            resp = requests.get(url, headers=headers, timeout=10)
            resp.raise_for_status()
            soup = BeautifulSoup(resp.text, 'html.parser')
            for script in soup(["script", "style", "nav", "footer", "iframe"]):
                script.extract()
            text = soup.get_text(separator=' ')
            return ' '.join(text.split())[:5000]
        except Exception:
            return ""

    def mine(self, raw_text: str, source_id: str):
        if not raw_text: return
        safe_id = "".join([c for c in source_id if c.isalnum() or c in ('_','-')])
        
        if safe_id in self.seen_urls:
            return
        
        print(f"[*] Mining Truth: {source_id}")
        content = self.analyze_content(raw_text)
        if not content: return

        try:
            # JSON 파싱 강화
            cleaned = content.strip()
            # 마크다운 코드 블록 제거 (혹시 몰라서)
            if cleaned.startswith("```json"): cleaned = cleaned[7:]
            if cleaned.endswith("```"): cleaned = cleaned[:-3]
            
            analysis_result = json.loads(cleaned)
            
            topo_data = {
                "meta": {
                    "source_id": safe_id,
                    "timestamp": datetime.now().isoformat(),
                    "miner_model": self.model_name
                },
                "attributes": analysis_result
            }
            
            output_dir = "data/inbox"
            os.makedirs(output_dir, exist_ok=True)
            with open(f"{output_dir}/{safe_id}.topo.json", 'w', encoding='utf-8') as f:
                json.dump(topo_data, f, indent=4, ensure_ascii=False)
            
            self.seen_urls.add(safe_id)
            print(f"[+] Materialized: {safe_id}")

        except Exception as e:
            print(f"[!] Parsing Failed: {e}")

    def scan_rss(self, rss_url: str):
        print(f"\n[Scheduler] Scanning Feed: {rss_url}")
        try:
            d = feedparser.parse(rss_url)
            for entry in d.entries[:3]:
                title_id = f"rss_{entry.title[:20].replace(' ', '_')}"
                if title_id in self.seen_urls: continue
                content = self.fetch_url(entry.link)
                self.mine(content, title_id)
        except Exception as e:
            print(f"[!] RSS Error: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--rss", type=str)
    parser.add_argument("--daemon", action="store_true")
    parser.add_argument("--interval", type=int, default=300)
    args = parser.parse_args()
    
    miner = TruthMiner()
    if args.daemon and args.rss:
        miner.scan_rss(args.rss)
        schedule.every(args.interval).seconds.do(miner.scan_rss, args.rss)
        while True:
            schedule.run_pending()
            time.sleep(1)