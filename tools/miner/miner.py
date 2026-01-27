# tools/miner/miner.py
import os
import json
import argparse
import time
from datetime import datetime
import requests
from bs4 import BeautifulSoup
import feedparser

# OpenAI API (없으면 분석 단계에서 에러 발생)
try:
    from openai import OpenAI
except ImportError:
    print("[!] Error: 'openai' library not installed.")
    exit(1)

class TruthMiner:
    def __init__(self, api_key: str = None, model: str = "gpt-4o"):
        self.client = OpenAI(api_key=api_key)
        self.model = model
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
                "dependency_sources": [list of strings, external citations/names mentioned],
                "circular_reasoning_detected": (bool)
            }
        }
        Do not include markdown formatting. Just raw JSON.
        """

    def fetch_url(self, url: str) -> str:
        """웹페이지의 본문 텍스트를 스크래핑합니다."""
        print(f"[*] Crawling URL: {url} ...")
        try:
            headers = {'User-Agent': 'TopoLogos/2.0'}
            resp = requests.get(url, headers=headers, timeout=10)
            resp.raise_for_status()
            
            soup = BeautifulSoup(resp.text, 'html.parser')
            
            # 불필요한 태그 제거 (Script, Style 등)
            for script in soup(["script", "style", "nav", "footer"]):
                script.extract()
                
            text = soup.get_text(separator=' ')
            # 공백 정리
            clean_text = ' '.join(text.split())
            return clean_text[:3000] # 너무 길면 LLM 비용 문제로 자름
        except Exception as e:
            print(f"[!] Crawling Failed: {e}")
            return ""

    def mine(self, raw_text: str, source_id: str) -> str:
        if not raw_text:
            print("[!] Empty text, skipping.")
            return None

        print(f"[*] Analyzing via {self.model}...")
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
            
            topo_data = {
                "meta": {
                    "source_id": source_id,
                    "timestamp": datetime.now().isoformat(),
                    "miner_model": self.model
                },
                "attributes": analysis_result
            }
            
            # 저장 경로 (프로젝트 루트의 data/inbox)
            # miner.py 위치가 tools/miner/ 라고 가정
            output_dir = os.path.join(os.path.dirname(__file__), "../../data/inbox")
            os.makedirs(output_dir, exist_ok=True)
            
            # 파일명에 특수문자 제거
            safe_id = "".join([c for c in source_id if c.isalnum() or c in ('_','-')])
            filename = os.path.join(output_dir, f"{safe_id}.topo.json")
            
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(topo_data, f, indent=4, ensure_ascii=False)
                
            print(f"[+] Materialized: {filename}")
            return filename

        except Exception as e:
            print(f"[!] Mining Failed: {e}")
            return None

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="TopoLogos Real Crawler")
    parser.add_argument("--url", type=str, help="Single URL to mine")
    parser.add_argument("--rss", type=str, help="RSS Feed URL to mine (fetches top 3 items)")
    parser.add_argument("--text", type=str, help="Manual text input")
    parser.add_argument("--id", type=str, default=f"manual_{int(time.time())}", help="Source ID")
    
    args = parser.parse_args()
    miner = TruthMiner()
    
    # 1. RSS 모드: 뉴스 피드를 순찰하며 자동 채굴
    if args.rss:
        print(f"[*] Scanning RSS Feed: {args.rss}")
        feed = feedparser.parse(args.rss)
        for entry in feed.entries[:3]: # 최신 3개만
            print(f"\n[>] Found: {entry.title}")
            # 링크 따라가서 본문 긁어오기
            content = miner.fetch_url(entry.link)
            if content:
                source_id = f"rss_{entry.title[:20].replace(' ', '_')}"
                miner.mine(content, source_id)
                time.sleep(1) # API 부하 방지

    # 2. URL 모드: 특정 웹페이지 채굴
    elif args.url:
        content = miner.fetch_url(args.url)
        if content:
            source_id = args.id if args.id != f"manual_{int(time.time())}" else "web_page_content"
            miner.mine(content, source_id)

    # 3. 텍스트 모드: 수동 입력
    elif args.text:
        miner.mine(args.text, args.id)

    else:
        print("Usage: python3 miner.py [--url URL | --rss RSS_URL | --text TEXT]")