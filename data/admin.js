let parsedProducts = [];

function showSection(id) {
    document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
    document.querySelectorAll('.admin-nav button').forEach(b => b.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    event.target.classList.add('active');
    
    if (id === 'images') loadImagesStatus();
    if (id === 'sales') loadSales();
}

// --- IMPORT LOGIC ---
async function processFile() {
    const fileInput = document.getElementById('file-import');
    if (!fileInput.files.length) return alert('Selecione um arquivo.');
    
    const file = fileInput.files[0];
    const isCSV = file.name.toLowerCase().endsWith('.csv');
    
    const reader = new FileReader();
    reader.onload = async function(e) {
        let rows = [];
        if (isCSV) {
            rows = parseCSV(e.target.result);
        } else {
            const data = new Uint8Array(e.target.result);
            const workbook = XLSX.read(data, {type: 'array'});
            const sheet = workbook.Sheets[workbook.SheetNames[0]];
            rows = XLSX.utils.sheet_to_json(sheet, {defval: ''});
        }
        await validateAndPreview(rows);
    };
    
    if (isCSV) {
        reader.readAsText(file, 'utf-8');
    } else {
        reader.readAsArrayBuffer(file);
    }
}

function parseCSV(text) {
    // Basic CSV parser
    const lines = text.split('\n').filter(l => l.trim().length > 0);
    if (lines.length < 2) return [];
    
    const headers = lines[0].split(',').map(h => h.trim().replace(/^"|"$/g, ''));
    const result = [];
    
    for (let i = 1; i < lines.length; i++) {
        const values = lines[i].split(',').map(v => v.trim().replace(/^"|"$/g, ''));
        let obj = {};
        headers.forEach((h, idx) => {
            obj[h] = values[idx] !== undefined ? values[idx] : '';
        });
        result.push(obj);
    }
    return result;
}

async function validateAndPreview(rows) {
    parsedProducts = [];
    let html = '<table class="admin-table"><thead><tr><th>Linha</th><th>Nome</th><th>Cat.</th><th>Custo</th><th>Venda</th><th>Estoque</th><th>Status</th></tr></thead><tbody>';
    
    for (let i = 0; i < rows.length; i++) {
        const r = rows[i];
        let isValid = true;
        let msg = '✓';
        let rowClass = 'valid-row';
        
        // Normalize keys lowercase
        const p = {};
        for(let key in r) p[key.toLowerCase().trim()] = r[key];

        if (!p.name) { isValid = false; msg = 'Nome vazio'; rowClass = 'error-row'; }
        const pCost = parseFloat(p.price_cost);
        const pSell = parseFloat(p.price_sell);
        const stock = parseInt(p.stock, 10);
        
        if (isNaN(pCost) || isNaN(pSell) || isNaN(stock)) {
            isValid = false; msg = 'Preço/Estoque inválidos'; rowClass = 'error-row';
        } else if (pSell < pCost) {
            msg = 'Venda < Custo (Aviso)'; rowClass = 'warning-row';
        }

        if (isValid || rowClass === 'warning-row') {
            parsedProducts.push({
                name: p.name,
                category: p.category || 'Geral',
                description: p.description || '',
                price_cost: pCost,
                price_sell: pSell,
                stock: stock,
                active: p.active !== '0' && p.active !== 0
            });
        }

        html += `<tr class="${rowClass}">
            <td>${i+2}</td>
            <td>${p.name || '-'}</td>
            <td>${p.category || '-'}</td>
            <td>${p.price_cost || '-'}</td>
            <td>${p.price_sell || '-'}</td>
            <td>${p.stock || '-'}</td>
            <td>${msg}</td>
        </tr>`;
    }
    html += '</tbody></table>';
    
    document.getElementById('preview-table').innerHTML = html;
    document.getElementById('preview-container').style.display = 'block';
}

async function confirmImport() {
    if (parsedProducts.length === 0) return alert('Nenhum produto válido para importar.');
    
    try {
        const res = await fetch('/api/products/bulk', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(parsedProducts)
        });
        
        const result = await res.json();
        alert(`Importação concluída!\nImportados: ${result.imported}\nErros: ${result.errors.length}`);
        document.getElementById('preview-container').style.display = 'none';
        document.getElementById('file-import').value = '';
    } catch (e) {
        alert('Erro ao enviar dados.');
    }
}

// --- IMAGES LOGIC ---
async function loadImagesStatus() {
    try {
        const pRes = await fetch('/api/products/all');
        const products = await pRes.json();
        
        const iRes = await fetch('/api/images');
        const imgsObj = await iRes.json();
        const images = imgsObj.images || [];

        let html = '';
        products.forEach(p => {
            const expectedName = p.slug + '.jpg';
            const expectedPng = p.slug + '.png';
            const exists = images.includes(expectedName) || images.includes(expectedPng);
            
            html += `<tr>
                <td>${p.name}</td>
                <td>${p.slug}</td>
                <td class="status-icon ${exists ? 'status-ok' : 'status-warn'}">${exists ? '✓ OK' : '⚠ Pendente'}</td>
            </tr>`;
        });
        
        document.getElementById('images-tbody').innerHTML = html;
    } catch(e) { console.error(e); }
}

async function uploadImages() {
    const input = document.getElementById('image-upload');
    const files = input.files;
    if (!files.length) return alert('Selecione arquivos.');

    let successCount = 0;
    for (let i = 0; i < files.length; i++) {
        const formData = new FormData();
        formData.append('file', files[i], files[i].name);
        
        try {
            await fetch('/api/products/image/upload', {
                method: 'POST',
                body: formData
            });
            successCount++;
        } catch (e) {
            console.error('Erro no upload de', files[i].name);
        }
    }
    alert(`${successCount} imagens enviadas.`);
    input.value = '';
    loadImagesStatus();
}

// --- SALES LOGIC ---
async function loadSales() {
    try {
        const res = await fetch('/api/sales');
        const sales = await res.json();
        
        let html = '';
        let grandTotal = 0;
        
        // Reverse to show newest first
        sales.reverse().forEach(s => {
            grandTotal += s.total;
            const dt = new Date(s.timestamp * 1000).toLocaleString('pt-BR');
            const itemsStr = s.items.map(i => `${i.qty}x ${i.name}`).join(', ');
            
            html += `<tr>
                <td>${dt}</td>
                <td style="font-weight:bold; color:var(--primary)">R$ ${s.total.toFixed(2)}</td>
                <td style="font-size: 0.8rem">${itemsStr}</td>
            </tr>`;
        });
        
        document.getElementById('sales-tbody').innerHTML = html;
        document.getElementById('total-sales').innerText = `Total: R$ ${grandTotal.toFixed(2)}`;
    } catch(e) { console.error(e); }
}
