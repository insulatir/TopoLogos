import json
import os
import sys

try:
    from graphviz import Digraph
except ImportError:
    print("[Error] Graphviz not installed. Run: pip install graphviz")
    sys.exit(1)

def visualize_knowledge_graph(db_path, output_filename="knowledge_graph"):
    """
    TopoLogos DB(JSON)를 읽어 Graphviz 시각화 파일로 변환합니다.
    """
    if not os.path.exists(db_path):
        print(f"[Error] DB file not found: {db_path}")
        return

    print(f"[*] Loading Knowledge Graph from: {db_path}")
    
    with open(db_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    nodes = data.get("nodes", {})
    edges = data.get("edges", [])

    if not nodes:
        print("[Warning] No verified facts found in DB.")
        return

    # 그래프 초기화 (방향성 그래프)
    dot = Digraph(comment='TopoLogos Knowledge Graph', format='png')
    dot.attr(rankdir='LR') # 좌우 방향 정렬
    dot.attr('node', shape='box', style='filled', color='lightblue')

    # 1. 노드 추가
    print(f"[*] Visualizing {len(nodes)} Verified Facts...")
    for node_id, node_data in nodes.items():
        score = node_data.get("score", 0)
        label = f"{node_id}\n(Integrity: {score})"
        
        # 점수에 따라 색상 변경 (높을수록 진한 초록)
        fillcolor = "#e5fffa"
        if score == 100: fillcolor = "#b2f7ef"
        
        dot.node(node_id, label=label, fillcolor=fillcolor)

    # 2. 엣지(관계) 추가
    print(f"[*] Mapping {len(edges)} Connections...")
    for edge in edges:
        src = edge["from"]
        dst = edge["to"]
        relation = edge["relation"]
        
        # 외부 의존성(Source) 노드가 DB에 없을 경우 자동 생성 (회색 점선)
        if dst not in nodes:
            dot.node(dst, label=dst, shape='ellipse', style='dashed', color='gray')
            
        dot.edge(src, dst, label=relation)

    # 3. 렌더링
    output_path = f"data/{output_filename}"
    dot.render(output_path, view=False)
    print(f"[Success] Graph rendered to: {output_path}.png")

if __name__ == "__main__":
    # DB 경로 설정 (프로젝트 루트 기준 data/topo_db.json)
    db_file = os.path.join("data", "topo_db.json")
    visualize_knowledge_graph(db_file)