let parsedProducts = [];
let allProducts = [];

document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('sales-date').valueAsDate = new Date();
    loadProducts();
});

function showSection(id) {
    document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
    document.querySelectorAll('.admin-nav button').forEach(b => b.classList.remove('active'));
    document.getElementById(id).classList.add('active');
    event.target.classList.add('active');
    
    if (id === 'produtos') loadProducts();
    if (id === 'images') { loadImagesStatus(); loadStorage(); }
    if (id === 'sales') loadSales();
}

// --- CRUD PRODUTOS ---
async function loadProducts() {
    try {
        const res = await fetch('/api/products/all');
        allProducts = await res.json();
        
        let html = '';
        allProducts.forEach(p => {
            if (!p) return;
            const imgPath = p.image ? p.image : '';
            html += `<tr>
                <td><img src="${imgPath}" width="40" height="40" style="object-fit:cover; border-radius:4px;" onerror="this.src='data:image/svg+xml;utf8,<svg xmlns=\\'http://www.w3.org/2000/svg\\' width=\\'40\\' height=\\'40\\'><rect width=\\'40\\' height=\\'40\\' fill=\\'%23ccc\\'/></svg>'"></td>
                <td>${p.name}<br><small>${p.category}</small></td>
                <td>${p.stock}</td>
                <td>R$ ${parseFloat(p.price_sell).toFixed(2)}</td>
                <td>
                    <button class="btn" style="padding:5px 10px;" onclick="editProduct('${p.id}')">✎</button>
                    <button class="btn btn-danger" style="padding:5px 10px;" onclick="deleteProduct('${p.id}')">✕</button>
                </td>
            </tr>`;
        });
        document.getElementById('products-tbody').innerHTML = html;
    } catch(e) { console.error(e); }
}

function calcMargin() {
    const cost = parseFloat(document.getElementById('prod-cost').value) || 0;
    const sell = parseFloat(document.getElementById('prod-sell').value) || 0;
    const ind = document.getElementById('margin-display');
    if(sell > 0) {
        const margin = ((sell - cost) / sell) * 100;
        ind.innerText = `Margem: ${margin.toFixed(1)}%`;
        ind.className = margin >= 0 ? 'margin-indicator margin-good' : 'margin-indicator margin-bad';
    } else {
        ind.innerText = `Margem: --%`;
    }
}

function openProductForm() {
    document.getElementById('product-form-container').style.display = 'block';
    document.getElementById('product-form').reset();
    document.getElementById('prod-id').value = '';
    document.getElementById('prod-img').value = '';
    document.getElementById('form-title').innerText = 'Novo Produto';
    calcMargin();
}

function closeProductForm() {
    document.getElementById('product-form-container').style.display = 'none';
}

function editProduct(id) {
    const p = allProducts.find(x => x.id === id);
    if(!p) return;
    document.getElementById('prod-id').value = p.id;
    document.getElementById('prod-name').value = p.name;
    document.getElementById('prod-cat').value = p.category;
    document.getElementById('prod-desc').value = p.description || '';
    document.getElementById('prod-cost').value = p.price_cost;
    document.getElementById('prod-sell').value = p.price_sell;
    document.getElementById('prod-stock').value = p.stock;
    document.getElementById('prod-img').value = p.image || '';
    document.getElementById('prod-active').value = p.active ? "true" : "false";
    document.getElementById('form-title').innerText = 'Editar Produto';
    calcMargin();
    document.getElementById('product-form-container').style.display = 'block';
}

async function saveProduct(e) {
    e.preventDefault();
    const id = document.getElementById('prod-id').value;
    const isEdit = !!id;
    
    const payload = {
        name: document.getElementById('prod-name').value,
        category: document.getElementById('prod-cat').value,
        description: document.getElementById('prod-desc').value,
        price_cost: parseFloat(document.getElementById('prod-cost').value),
        price_sell: parseFloat(document.getElementById('prod-sell').value),
        stock: parseInt(document.getElementById('prod-stock').value),
        active: document.getElementById('prod-active').value === "true"
    };

    const imgVal = document.getElementById('prod-img').value.trim();
    if(imgVal) payload.image = imgVal;

    if(isEdit) payload.id = id;

    const url = isEdit ? `/api/products/${id}` : '/api/products';
    const method = isEdit ? 'PUT' : 'POST';

    try {
        const res = await fetch(url, {
            method: method,
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(payload)
        });
        if(res.ok) {
            alert('Salvo com sucesso!');
            closeProductForm();
            loadProducts();
        } else {
            alert('Erro ao salvar.');
        }
    } catch(err) { alert('Erro de rede.'); }
}

async function deleteProduct(id) {
    if(!confirm('Certeza que deseja excluir?')) return;
    try {
        const res = await fetch(`/api/products/${id}`, { method: 'DELETE' });
        if(res.ok) { loadProducts(); } else { alert('Erro ao excluir.'); }
    } catch(e) { alert('Erro de rede.'); }
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
            await validateAndPreview(rows);
        } else {
            if(typeof XLSX === 'undefined') return alert('SheetJS não carregado. Verifique data/libs/xlsx.full.min.js');
            const data = new Uint8Array(e.target.result);
            const workbook = XLSX.read(data, {type: 'array'});
            const sheet = workbook.Sheets[workbook.SheetNames[0]];
            rows = XLSX.utils.sheet_to_json(sheet, {defval: ''});
            await validateAndPreview(rows);
        }
    };
    if (isCSV) reader.readAsText(file, 'utf-8');
    else reader.readAsArrayBuffer(file);
}

function parseCSV(text) {
    const lines = text.split('\n').filter(l => l.trim().length > 0);
    if (lines.length < 2) return [];
    const headers = lines[0].split(/[;,]/).map(h => h.trim().replace(/^"|"$/g, ''));
    const result = [];
    for (let i = 1; i < lines.length; i++) {
        const values = lines[i].split(/[;,]/).map(v => v.trim().replace(/^"|"$/g, ''));
        let obj = {};
        headers.forEach((h, idx) => { obj[h] = values[idx] !== undefined ? values[idx] : ''; });
        result.push(obj);
    }
    return result;
}

async function validateAndPreview(rows) {
    parsedProducts = [];
    let html = '<table class="admin-table"><thead><tr><th>Linha</th><th>Nome</th><th>Cat.</th><th>Preços</th><th>Estoque</th><th>Img</th><th>Status</th></tr></thead><tbody>';
    
    for (let i = 0; i < rows.length; i++) {
        const r = rows[i];
        let isValid = true;
        let msg = '✓';
        let rowClass = '';
        
        const p = {};
        for(let key in r) p[key.toLowerCase().trim()] = r[key];

        if (!p.name) { isValid = false; msg = 'Nome vazio'; rowClass = 'status-error'; }
        const pCost = parseFloat(p.price_cost);
        const pSell = parseFloat(p.price_sell);
        const stock = parseInt(p.stock, 10);
        
        if (isNaN(pCost) || isNaN(pSell) || isNaN(stock) || pCost < 0 || pSell < 0 || stock < 0) {
            isValid = false; msg = 'Valores inválidos'; rowClass = 'status-error';
        } else if (pSell < pCost) {
            msg = 'Venda < Custo'; rowClass = 'status-warn';
        }

        let imgStatus = '-';
        if (p.name) {
            let slug = p.name.toLowerCase().normalize("NFD").replace(/[\u0300-\u036f]/g, "").replace(/[^a-z0-9]/g, '_').replace(/_+/g, '_').replace(/_$/, '');
            try {
                const imgRes = await fetch(`/api/images/check?slug=${slug}`);
                const imgData = await imgRes.json();
                imgStatus = imgData.exists ? '<span class="status-ok">✓</span>' : '<span class="status-warn">⚠</span>';
            } catch(e) { imgStatus = '?'; }
        }

        if (isValid || rowClass === 'status-warn') {
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

        html += `<tr>
            <td>${i+2}</td>
            <td>${p.name || '-'}</td>
            <td>${p.category || '-'}</td>
            <td>C:${pCost} V:${pSell}</td>
            <td>${stock}</td>
            <td>${imgStatus}</td>
            <td class="${rowClass}">${msg}</td>
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
        
        if (!res.ok) {
            return alert(`Falha na importação: ${result.error || 'Erro desconhecido'}`);
        }
        
        alert(`Importação concluída!\nImportados: ${result.imported}\nErros: ${result.errors ? result.errors.length : 0}`);
        document.getElementById('preview-container').style.display = 'none';
        document.getElementById('file-import').value = '';
        loadProducts();
    } catch (e) { alert('Erro ao enviar dados. O payload pode ser grande demais para o ESP32.'); }
}

// --- IMAGES LOGIC ---
async function loadStorage() {
    try {
        const res = await fetch('/api/system/storage');
        const d = await res.json();
        const mbf = (d.free_bytes / 1024 / 1024).toFixed(2);
        const mbu = (d.used_bytes / 1024 / 1024).toFixed(2);
        document.getElementById('storage-info').innerText = `Armazenamento: ${mbu}MB Usados / ${mbf}MB Livres`;
    } catch(e){}
}

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
                <td class="${exists ? 'status-ok' : 'status-warn'}">${exists ? '✓ Encontrada' : '⚠ Pendente'}</td>
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
        if(files[i].size > 102400) {
            alert(`Aviso: ${files[i].name} excede 100KB e será ignorado.`);
            continue;
        }
        const formData = new FormData();
        formData.append('file', files[i], files[i].name);
        
        try {
            const res = await fetch('/api/products/image/upload', { method: 'POST', body: formData });
            if(res.ok) successCount++;
            else console.log('Erro no upload de', files[i].name);
        } catch (e) { console.error('Erro no upload de', files[i].name); }
    }
    alert(`${successCount} imagens enviadas com sucesso.`);
    input.value = '';
    loadImagesStatus();
    loadStorage();
}

// --- SALES LOGIC ---
async function loadSales() {
    const dateInput = document.getElementById('sales-date').value;
    if(!dateInput) return;

    try {
        const sumRes = await fetch(`/api/sales/summary?date=${dateInput}`);
        const summary = await sumRes.json();
        
        document.getElementById('card-total').innerText = `R$ ${summary.total_revenue.toFixed(2)}`;
        document.getElementById('card-count').innerText = summary.total_sales;
        
        let topProd = '-';
        let maxQty = 0;
        for(let key in summary.items_sold) {
            if(summary.items_sold[key] > maxQty) {
                maxQty = summary.items_sold[key];
                topProd = key;
            }
        }
        document.getElementById('card-top').innerText = topProd;

        const res = await fetch(`/api/sales?date=${dateInput}`);
        const sales = await res.json();
        let html = '';
        sales.reverse().forEach(s => {
            const dt = new Date(s.timestamp * 1000).toLocaleTimeString('pt-BR');
            const itemsStr = s.items.map(i => `${i.qty}x ${i.name}`).join(', ');
            html += `<tr>
                <td>${dt}</td>
                <td style="font-weight:bold;">R$ ${parseFloat(s.total).toFixed(2)}</td>
                <td style="font-size: 0.8rem">${itemsStr}</td>
            </tr>`;
        });
        document.getElementById('sales-tbody').innerHTML = html || '<tr><td colspan="3">Nenhuma venda encontrada para esta data.</td></tr>';
    } catch(e) { console.error(e); }
}
