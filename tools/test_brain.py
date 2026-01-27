import requests
import json

# Docker 내부에서는 'localhost'가 아니라 서비스 이름('brain')으로 찾습니다.
OLLAMA_URL = "http://brain:11434/api/generate"

def talk_to_brain(prompt):
    payload = {
        "model": "mistral",
        "prompt": prompt,
        "stream": False
    }
    
    try:
        print(f"🧠 뇌에게 질문 중: {prompt}")
        response = requests.post(OLLAMA_URL, json=payload)
        response.raise_for_status()
        
        result = response.json()
        return result['response']
        
    except Exception as e:
        return f"💀 뇌가 응답하지 않습니다: {str(e)}"

if __name__ == "__main__":
    answer = talk_to_brain("Hello! Who are you? Answer in one sentence.")
    print(f"🤖 답변: {answer}")