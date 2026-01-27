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
        # Local Brain (Ollama) 설정
        self.brain_url = "http://brain:11434/api/generate"
        self.model_name = "mistral"
        
        # 중복 체크 장부
        self.seen_urls = set()
        
        # 시스템 프롬프트 (분석 가이드라인)
        self.system_prompt = (
            "You are TopoLogos Sentinel. Analyze news text and extract facts. "
            "Return ONLY a JSON object with: {'summary': '...', 'score': 0-100, "
            "'structure': {'dependency_sources': []}}"
        )
        
        self.load_history()
        print("[*] TruthMiner Initialized with Local Brain (Mistral).")

    def analyze_content(self, text):
        """로컬 LLM(Ollama)에게 분석을 요청합니다."""
        # 시스템 프롬프트와 본문을 결합
        full_prompt = f"{self.system_prompt}\n\nText to analyze:\n{text}"
        
        # flush=True를 넣어야 로그가 즉시 찍힙니다.
        print(f"🧠 Asking Local Brain (Mistral)... (본문 길이: {len(text)})", flush=True)

        payload = {
            "model": self.model_name,
            "prompt": full_prompt,
            "stream": False,
            "format": "json" 
        }
        
        try:
            print(f"🧠 Asking Local Brain (Mistral)...")
            response = requests.post(self.brain_url, json=payload, timeout=600)
            response.raise_for_status()
            print(f"✅ Brain responded successfully!", flush=True)
            result = response.json()
            return result['response']
        except requests.exceptions.Timeout:
            print(f"💀 Brain is too slow! Timeout occurred.", flush=True)
            return None
        except Exception as e:
            print(f"[!] Error: {e}", flush=True)
            return None

    def load_history(self):
        inbox_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
        if os.path.exists(inbox_dir):
            for f in os.listdir(inbox_dir):
                if f.endswith(".topo.json"):
                    self.seen_urls.add(f.replace(".topo.json", ""))

    def fetch_url(self, url: str) -> str:
        print(f"[*] Crawling URL: {url} ...")
        try:
            headers = {'User-Agent': 'TopoLogos/3.0'}
            resp = requests.get(url, headers=headers, timeout=10)
            resp.raise_for_status()
            soup = BeautifulSoup(resp.text, 'html.parser')
            for script in soup(["script", "style", "nav", "footer", "iframe"]):
                script.extract()
            text = soup.get_text(separator=' ')
            return ' '.join(text.split())[:5000]
        except Exception as e:
            print(f"[!] Crawling Failed: {e}")
            return ""

    def mine(self, raw_text: str, source_id: str, depth: int = 0, max_depth: int = 1):
        if not raw_text: return
        safe_id = "".join([c for c in source_id if c.isalnum() or c in ('_','-')])
        
        if safe_id in self.seen_urls:
            print(f"[-] Skipping known source: {safe_id}")
            return
        
        print(f"[*] Mining Truth ({depth}/{max_depth}): {source_id}")
        
        # [수정] OpenAI 대신 로컬 analyze_content 사용
        content = self.analyze_content(raw_text)
        if not content: return

        try:
            analysis_result = json.loads(content)

            # Mistral이 준 답변에서 JSON만 쏙 뽑아내는 정규식
            import re
            json_match = re.search(r'\{.*\}', content, re.DOTALL)
            if json_match:
                analysis_result = json.loads(json_match.group(1))
            else:
                # JSON 형식이 아예 없으면 원본이라도 파싱 시도
                analysis_result = json.loads(content)
            
            topo_data = {
                "meta": {
                    "source_id": safe_id,
                    "timestamp": datetime.now().isoformat(),
                    "miner_model": self.model_name,
                    "depth": depth
                },
                "attributes": analysis_result
            }
            
            output_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
            os.makedirs(output_dir, exist_ok=True)
            filename = os.path.join(output_dir, f"{safe_id}.topo.json")
            
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(topo_data, f, indent=4, ensure_ascii=False)
            
            self.seen_urls.add(safe_id)
            print(f"[+] Materialized: {filename}")

            # 재귀적 크롤링
            if depth < max_depth:
                sources = analysis_result.get("structure", {}).get("dependency_sources", [])
                for src in sources:
                    if re.match(r'https?://', src):
                        child_text = self.fetch_url(src)
                        child_id = f"{safe_id}_ref_{abs(hash(src)) % 10000}"
                        self.mine(child_text, child_id, depth + 1, max_depth)    

        except Exception as e:
            print(f"[!] JSON 추출 실패: {e}")
            # 분석에 실패해도 최소한의 정보는 남김
            analysis_result = {"summary": "Analysis failed", "score": 0}

    def scan_rss(self, rss_url: str):
        print(f"\n[Scheduler] Scanning Feed: {rss_url}")
        d = feedparser.parse(rss_url)
        for entry in d.entries[:3]:
            title_id = f"rss_{entry.title[:20].replace(' ', '_')}"
            # 중복 체크
            if title_id in self.seen_urls:
                continue
            content = self.fetch_url(entry.link)
            self.mine(content, title_id)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TopoLogos Autonomous Miner")
    parser.add_argument("--rss", type=str, help="RSS Feed URL to monitor")
    parser.add_argument("--daemon", action="store_true", help="Run in continuous loop mode")
    parser.add_argument("--interval", type=int, default=300, help="Interval (sec)")
    parser.add_argument("--url", type=str)
    
    args = parser.parse_args()
    miner = TruthMiner()

    if args.daemon and args.rss:
        print(f"[*] Starting TopoLogos Sentinel Daemon.")
        miner.scan_rss(args.rss)
        schedule.every(args.interval).seconds.do(miner.scan_rss, args.rss)
        while True:
            schedule.run_pending()
            time.sleep(1)
    elif args.url:
        content = miner.fetch_url(args.url)
        miner.mine(content, f"manual_{int(time.time())}")
    elif args.rss:
        miner.scan_rss(args.rss)