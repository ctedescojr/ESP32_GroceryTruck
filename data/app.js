let products = [];
let cart = [];

async function loadProducts() {
    try {
        const [prodRes, imgRes] = await Promise.all([
            fetch('/api/products'),
            fetch('/api/images')
        ]);
        products = await prodRes.json();
        const images = (await imgRes.json()).images || [];
        renderProducts(images);
    } catch (e) { console.error("Erro ao carregar produtos:", e); }
}

function renderProducts(availableImages = []) {
    const container = document.getElementById('store-container');
    const nav = document.getElementById('category-nav');
    container.innerHTML = '';
    nav.innerHTML = '';
    
    const categories = {};
    products.forEach(p => {
        if (!p) return;
        if (!categories[p.category]) categories[p.category] = [];
        categories[p.category].push(p);
    });

    for (const cat in categories) {
        const anchorId = `cat-${cat.replace(/[\s\W]+/g, '-')}`;
        
        const navLink = document.createElement('a');
        navLink.href = `#${anchorId}`;
        navLink.innerText = cat;
        nav.appendChild(navLink);

        const title = document.createElement('h2');
        title.className = 'category-title';
        title.id = anchorId;
        title.innerText = cat;
        title.style.marginTop = '20px';
        container.appendChild(title);

        const grid = document.createElement('div');
        grid.className = 'product-grid';
        
        categories[cat].forEach(p => {
            const isOutOfStock = p.stock <= 0;
            const inCart = cart.find(c => c.product_id === p.id);
            const qtyInCart = inCart ? inCart.qty : 0;
            const available = p.stock - qtyInCart;

            // Intelligent Path Selection
            let imgPath = p.image || '';
            if (!imgPath || !imgPath.includes('.')) {
                const extensions = ['.png', '.jpg', '.jpeg'];
                for (let ext of extensions) {
                    if (availableImages.includes(p.slug + ext)) {
                        imgPath = '/img/' + p.slug + ext;
                        break;
                    }
                }
            }
            if (!imgPath) imgPath = "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='100' height='100'><rect width='100' height='100' fill='%23eee'/><text x='50%' y='50%' dominant-baseline='middle' text-anchor='middle' fill='%23999'>Sem Foto</text></svg>";

            const card = document.createElement('div');
            card.className = 'product-card';
            card.innerHTML = `
                <img src="${imgPath}" class="product-img">
                <div class="product-info">
                    <div class="product-name" style="font-weight:bold; margin-bottom:5px;">${p.name}</div>
                    <div style="font-size:0.8em; color:#666; margin-bottom:5px; min-height:30px;">${p.description || ''}</div>
                    <div class="product-price">R$ ${parseFloat(p.price_sell).toFixed(2)}</div>
                </div>
                <button class="btn" style="margin-top:auto;" ${available <= 0 ? 'disabled' : ''} onclick="addToCart('${p.id}')">
                    ${isOutOfStock ? 'Esgotado' : 'Adicionar'}
                </button>
            `;
            grid.appendChild(card);
        });
        container.appendChild(grid);
    }
}

function addToCart(id) {
    const product = products.find(p => p.id === id);
    if (!product) return;

    const item = cart.find(c => c.product_id === id);
    const currentQty = item ? item.qty : 0;

    if (currentQty >= product.stock) {
        alert('Estoque limite atingido!');
        return;
    }

    if (item) item.qty++;
    else cart.push({ product_id: product.id, name: product.name, qty: 1, unit_price: product.price_sell });
    
    updateCartUI();
    renderProducts();
}

function updateCartQty(id, delta) {
    const item = cart.find(c => c.product_id === id);
    if(!item) return;
    
    const product = products.find(p => p.id === id);
    if(delta > 0 && item.qty >= product.stock) return alert('Estoque limite atingido!');
    
    item.qty += delta;
    if(item.qty <= 0) cart = cart.filter(c => c.product_id !== id);
    
    updateCartUI();
    renderProducts();
    renderCartModal();
}

function updateCartUI() {
    const bar = document.getElementById('cart-bar');
    if (cart.length === 0) { bar.style.display = 'none'; closeCart(); return; }

    bar.style.display = 'flex';
    let total = 0; let count = 0;
    cart.forEach(item => { total += item.qty * item.unit_price; count += item.qty; });

    document.getElementById('cart-count').innerText = count;
    document.getElementById('cart-total').innerText = `R$ ${total.toFixed(2)}`;
}

function openCart() {
    document.getElementById('cart-modal').style.display = 'flex';
    renderCartModal();
}

function closeCart() {
    document.getElementById('cart-modal').style.display = 'none';
}

function renderCartModal() {
    const container = document.getElementById('cart-items');
    container.innerHTML = '';
    let total = 0;
    
    cart.forEach(item => {
        total += item.qty * item.unit_price;
        container.innerHTML += `
            <div class="cart-item">
                <div>
                    <div style="font-weight:bold;">${item.name}</div>
                    <div style="color:#666;">R$ ${item.unit_price.toFixed(2)}</div>
                </div>
                <div class="qty-controls">
                    <button onclick="updateCartQty('${item.product_id}', -1)">-</button>
                    <span>${item.qty}</span>
                    <button onclick="updateCartQty('${item.product_id}', 1)">+</button>
                </div>
            </div>
        `;
    });
    document.getElementById('modal-total').innerText = `R$ ${total.toFixed(2)}`;
}

async function checkout() {
    if (cart.length === 0) return;
    
    let total = cart.reduce((acc, item) => acc + (item.qty * item.unit_price), 0);
    const today = new Date();
    const dateStr = today.toISOString().split('T')[0]; // YYYY-MM-DD
    
    const payload = { 
        items: cart, 
        total: total,
        date: dateStr,
        timestamp: Math.floor(today.getTime() / 1000)
    };

    try {
        const res = await fetch('/api/sales', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        
        if (res.ok) {
            alert('Compra finalizada com sucesso!');
            cart = [];
            updateCartUI();
            closeCart();
            loadProducts();
        } else { alert('Erro ao finalizar compra.'); }
    } catch (e) { alert('Erro de conexão.'); }
}

loadProducts();
