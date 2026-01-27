import sqlite3
import os
from graphviz import Digraph

def visualize_knowledge_graph(db_path, output_filename="knowledge_graph"):
    # [변경] SQLite 연결
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    dot = Digraph(comment='TopoLogos Knowledge Graph', format='png')
    dot.attr(rankdir='LR')
    # [설정] 한글 깨짐 방지
    dot.attr('node', fontname='NanumGothic') 

    # 1. 노드 가져오기 (SQL)
    cursor.execute("SELECT id, score FROM nodes")
    nodes = cursor.fetchall() # 리스트 형태 [(id, score), ...]
    
    existing_ids = set()
    for node_id, score in nodes:
        existing_ids.add(node_id)
        label = f"{node_id}\n({int(score)})"
        color = "#b2f7ef" if score >= 80 else "#ffcccc"
        dot.node(node_id, label=label, fillcolor=color, style='filled')

    # 2. 엣지 가져오기 (SQL)
    cursor.execute("SELECT source, target, relation FROM edges")
    edges = cursor.fetchall()
    
    for src, dst, rel in edges:
        if dst not in existing_ids:
            dot.node(dst, label=dst, style='dashed')
        dot.edge(src, dst, label=rel)
        
    dot.render(f"data/{output_filename}", cleanup=True)
    print(f"[Success] Graph rendered to data/{output_filename}.png")

if __name__ == "__main__":
    # [변경] 파일명 .sqlite 로 수정
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    db_file = os.path.join(base_dir, "data/topo_db.sqlite")
    visualize_knowledge_graph(db_file)