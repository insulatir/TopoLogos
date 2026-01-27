import os
import json
import argparse
import time
import re
from datetime import datetime
import requests
from bs4 import BeautifulSoup
import feedparser
import schedule # pip install schedule

try:
    from openai import OpenAI
except ImportError:
    print("[!] Error: 'openai' library not installed.")
    exit(1)

class TruthMiner:
    def __init__(self, api_key: str = None, model: str = "gpt-4o"):
        self.client = OpenAI(api_key=api_key)
        self.model = model
        # 이미 처리한 URL을 기억하여 중복 방지
        self.seen_urls = set()
        self.load_history()

        self.system_prompt = """
        You are the 'Sense Organ' of the TopoLogos engine.
        Analyze the input text based on 'Structural Existentialism'.
        
        Return the result strictly in JSON format with the following schema:
        {
            "physics": {
                "input_energy": (float 0.0-1.0),
                "promised_reward": (float),
                "text_clarity_score": (float 0.0-1.0)
            },
            "psychology": {
                "urgency_keywords_count": (int),
                "call_to_action": (bool),
                "absolute_terms_count": (int)
            },
            "structure": {
                "dependency_sources": [list of strings, specific URLs or citations found in text],
                "circular_reasoning_detected": (bool)
            }
        }
        IMPORTANT: If you find external links or citations in the text, put them in 'dependency_sources'.
        Do not include markdown formatting. Just raw JSON.
        """

    def load_history(self):
        # data/inbox 폴더를 스캔하여 이미 수집한 ID 로드
        inbox_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
        if os.path.exists(inbox_dir):
            for f in os.listdir(inbox_dir):
                if f.endswith(".topo.json"):
                    # 파일명에서 ID 추출 (단순화)
                    self.seen_urls.add(f.replace(".topo.json", ""))

    def fetch_url(self, url: str) -> str:
        """웹페이지의 본문 텍스트를 스크래핑합니다."""
        print(f"[*] Crawling URL: {url} ...")
        try:
            headers = {'User-Agent': 'TopoLogos/3.0'}
            resp = requests.get(url, headers=headers, timeout=10)
            resp.raise_for_status()
            
            soup = BeautifulSoup(resp.text, 'html.parser')
            for script in soup(["script", "style", "nav", "footer", "iframe"]):
                script.extract()
                
            text = soup.get_text(separator=' ')
            clean_text = ' '.join(text.split())
            return clean_text[:5000] # 분석 길이 제한
        except Exception as e:
            print(f"[!] Crawling Failed: {e}")
            return ""

    def mine(self, raw_text: str, source_id: str, depth: int = 0, max_depth: int = 1):
        if not raw_text: return

        # 중복 방지 (ID 기준)
        safe_id = "".join([c for c in source_id if c.isalnum() or c in ('_','-')])
        if safe_id in self.seen_urls:
            print(f"[-] Skipping known source: {safe_id}")
            return
        
        print(f"[*] Mining Truth ({depth}/{max_depth}): {source_id}")
        
        try:
            response = self.client.chat.completions.create(
                model=self.model,
                messages=[
                    {"role": "system", "content": self.system_prompt},
                    {"role": "user", "content": f"Analyze this text:\n{raw_text}"}
                ],
                temperature=0.0
            )
            content = response.choices[0].message.content.strip()
            if content.startswith("```json"):
                content = content.replace("```json", "").replace("```", "")
            
            analysis_result = json.loads(content)
            
            # 메타데이터 생성
            topo_data = {
                "meta": {
                    "source_id": safe_id,
                    "timestamp": datetime.now().isoformat(),
                    "miner_model": self.model,
                    "depth": depth
                },
                "attributes": analysis_result
            }
            
            # 저장
            output_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
            os.makedirs(output_dir, exist_ok=True)
            filename = os.path.join(output_dir, f"{safe_id}.topo.json")
            
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(topo_data, f, indent=4, ensure_ascii=False)
            
            self.seen_urls.add(safe_id)
            print(f"[+] Materialized: {filename}")

            # [Recursive Crawling] 출처가 URL이라면 파고들기
            if depth < max_depth:
                sources = analysis_result.get("structure", {}).get("dependency_sources", [])
                for src in sources:
                    # 간단한 URL 감지 정규식
                    if re.match(r'https?://', src):
                        print(f"[>] Recursive Discovery: Found source link {src}")
                        child_text = self.fetch_url(src)
                        # 자식 노드 ID 생성 (부모ID_child_...)
                        child_id = f"{safe_id}_ref_{abs(hash(src)) % 10000}"
                        self.mine(child_text, child_id, depth + 1, max_depth)

        except Exception as e:
            print(f"[!] Mining Failed: {e}")

    def scan_rss(self, rss_url: str):
        print(f"\n[Scheduler] Scanning Feed: {rss_url}")
        d = feedparser.parse(rss_url)
        for entry in d.entries[:3]: # 최신 3개
            title_id = f"rss_{entry.title[:20].replace(' ', '_')}"
            # URL로 이미 본 건지 확인 (ID가 변할 수 있으므로)
            if any(entry.link in s for s in self.seen_urls): 
                continue

            content = self.fetch_url(entry.link)
            self.mine(content, title_id)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TopoLogos Autonomous Miner")
    parser.add_argument("--rss", type=str, help="RSS Feed URL to monitor")
    parser.add_argument("--daemon", action="store_true", help="Run in continuous loop mode")
    parser.add_argument("--interval", type=int, default=60, help="Scan interval in seconds (default: 60)")
    
    # Manual mode args
    parser.add_argument("--url", type=str, help="Single URL to mine")
    parser.add_argument("--text", type=str, help="Manual text input")
    parser.add_argument("--id", type=str, default=f"manual_{int(time.time())}")

    args = parser.parse_args()
    miner = TruthMiner()

    # 1. 데몬 모드 (자동화)
    if args.daemon and args.rss:
        print(f"[*] Starting TopoLogos Sentinel Daemon.")
        print(f"[*] Target: {args.rss}")
        print(f"[*] Interval: {args.interval} seconds")
        
        # 즉시 1회 실행
        miner.scan_rss(args.rss)
        
        # 스케줄 등록
        schedule.every(args.interval).seconds.do(miner.scan_rss, args.rss)
        
        while True:
            schedule.run_pending()
            time.sleep(1)

    # 2. 수동 모드
    elif args.url:
        content = miner.fetch_url(args.url)
        miner.mine(content, args.id)
    elif args.rss:
        miner.scan_rss(args.rss)
    elif args.text:
        miner.mine(args.text, args.id)
    else:
        print("Usage: python3 miner.py [--daemon --rss URL] | [--url URL]")