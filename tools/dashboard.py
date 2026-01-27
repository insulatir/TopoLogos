# tools/dashboard.py
import sqlite3
import json
import os
from flask import Flask, jsonify, render_template_string

app = Flask(__name__)

# DB 경로 설정
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DB_PATH = os.path.join(BASE_DIR, "data/topo_db.sqlite")

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# --- HTML Template (Embedded for simplicity) ---
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>TopoLogos Knowledge Dashboard</title>
    <script type="text/javascript" src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
    <style>
        body { margin: 0; padding: 0; font-family: 'Segoe UI', sans-serif; overflow: hidden; }
        #network { width: 75vw; height: 100vh; float: left; background: #f0f2f5; }
        #sidebar { width: 25vw; height: 100vh; float: right; background: #fff; border-left: 1px solid #ccc; overflow-y: auto; padding: 20px; box-sizing: border-box; box-shadow: -2px 0 5px rgba(0,0,0,0.05); }
        h2 { margin-top: 0; color: #333; }
        .score-badge { display: inline-block; padding: 5px 10px; border-radius: 15px; color: white; font-weight: bold; font-size: 0.9em; }
        .truth { background-color: #2196F3; }
        .scam { background-color: #F44336; }
        pre { background: #f8f9fa; padding: 10px; border-radius: 5px; white-space: pre-wrap; font-size: 0.85em; color: #444; border: 1px solid #eee; }
        .meta { color: #888; font-size: 0.8em; margin-bottom: 15px; }
        .stat-item { margin-bottom: 5px; }
    </style>
</head>
<body>
    <div id="network"></div>
    <div id="sidebar">
        <h2>TopoLogos Inspector</h2>
        <div id="details">
            <p style="color: #666;">Click on a node to view its structural analysis.</p>
        </div>
    </div>

    <script type="text/javascript">
        // 1. Fetch Data
        fetch('/api/graph')
            .then(response => response.json())
            .then(data => {
                const container = document.getElementById('network');
                
                // 2. Setup Vis.js Data
                const nodes = new vis.DataSet(data.nodes);
                const edges = new vis.DataSet(data.edges);
                
                const options = {
                    nodes: {
                        shape: 'dot',
                        size: 20,
                        font: { size: 14, face: 'NanumGothic' },
                        borderWidth: 2
                    },
                    edges: {
                        width: 1,
                        arrows: 'to',
                        color: { color: '#ccc' },
                        smooth: { type: 'continuous' }
                    },
                    physics: {
                        stabilization: false,
                        barnesHut: {
                            gravitationalConstant: -8000,
                            springConstant: 0.04,
                            springLength: 95
                        }
                    },
                    interaction: { hover: true, tooltipDelay: 200 }
                };

                const network = new vis.Network(container, { nodes, edges }, options);

                // 3. Click Event Handler
                network.on("click", function (params) {
                    if (params.nodes.length > 0) {
                        const nodeId = params.nodes[0];
                        const nodeData = nodes.get(nodeId);
                        showDetails(nodeData);
                    }
                });
            });

        function showDetails(node) {
            const sidebar = document.getElementById('details');
            if (!node.payload) return;

            const isTruth = node.score >= 50;
            const badgeClass = isTruth ? 'truth' : 'scam';
            const statusText = isTruth ? 'VERIFIED TRUTH' : 'SUSPICIOUS / SCAM';
            
            // JSON Payload Parsing
            let analysis = {};
            try { analysis = JSON.parse(node.payload); } catch(e) {}

            let html = `
                <div style="margin-bottom: 15px;">
                    <span class="score-badge ${badgeClass}">${statusText}</span>
                    <span style="float: right; font-weight: bold; color: #555;">Score: ${node.score}</span>
                </div>
                <h3>${node.label}</h3>
                <div class="meta">
                    Type: ${node.group}<br>
                    ID: ${node.id}
                </div>
            `;

            // Physics / Psych / Structure Details
            if (analysis.attributes) {
                const attr = analysis.attributes;
                html += `<h4>🧠 Psychology</h4>`;
                html += `<div class="stat-item">Urgency: ${attr.psychology?.urgency_keywords_count || 0}</div>`;
                html += `<div class="stat-item">Absolutes: ${attr.psychology?.absolute_terms_count || 0}</div>`;
                
                html += `<h4>🏗️ Structure</h4>`;
                html += `<div class="stat-item">Sources: ${(attr.structure?.dependency_sources || []).length}</div>`;
                
                html += `<h4>⚛️ Physics</h4>`;
                html += `<div class="stat-item">Entropy (Clarity): ${attr.physics?.text_clarity_score || 0}</div>`;
            }

            html += `<h4>📜 Raw Analysis</h4>`;
            html += `<pre>${JSON.stringify(analysis, null, 2)}</pre>`;

            sidebar.innerHTML = html;
        }
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

# tools/dashboard.py 내부의 get_graph 함수 수정

@app.route('/api/graph')
def get_graph():
    conn = get_db_connection()
    
    # 1. 정식 노드 가져오기
    nodes_db = conn.execute('SELECT id, type, score, payload FROM nodes').fetchall()
    nodes = []
    existing_ids = set() # 이미 존재하는 ID 기록용
    
    for row in nodes_db:
        score = row['score']
        color = "#2196F3" if score >= 80 else ("#FF9800" if score >= 50 else "#F44336")
        
        nodes.append({
            "id": row['id'],
            "label": row['id'],
            "group": row['type'],
            "score": score,
            "color": color,
            "payload": row['payload']
        })
        existing_ids.add(row['id'])

    # 2. 엣지 가져오기 + [핵심] 없는 노드(외부 출처) 생성하기
    edges_db = conn.execute('SELECT source, target, relation FROM edges').fetchall()
    edges = []
    
    for row in edges_db:
        target_id = row['target']
        
        # [Fix] 타겟 노드가 정식 노드 리스트에 없다면 "외부 출처(External)"로 임시 생성
        if target_id not in existing_ids:
            nodes.append({
                "id": target_id,
                "label": target_id,
                "group": "ExternalSource", # 그룹 구분
                "score": 0,
                "color": "#e0e0e0",        # 회색으로 표시
                "shape": "box",            # 모양도 다르게 (네모)
                "payload": "{}"            # 빈 데이터
            })
            existing_ids.add(target_id) # 중복 생성 방지

        edges.append({
            "from": row['source'],
            "to": row['target'],
            "label": row['relation']
        })

    conn.close()
    return jsonify({"nodes": nodes, "edges": edges})

if __name__ == '__main__':
    print("[*] TopoLogos Dashboard running at http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)