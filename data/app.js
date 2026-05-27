let products = [];
let cart = [];

async function loadProducts() {
    try {
        const res = await fetch('/api/products');
        products = await res.json();
        renderProducts();
    } catch (e) {
        console.error("Erro ao carregar produtos:", e);
    }
}

function renderProducts() {
    const container = document.getElementById('store-container');
    container.innerHTML = '';
    
    // Group by category
    const categories = {};
    products.forEach(p => {
        if (!categories[p.category]) categories[p.category] = [];
        categories[p.category].push(p);
    });

    for (const cat in categories) {
        const title = document.createElement('h2');
        title.className = 'category-title';
        title.innerText = cat;
        container.appendChild(title);

        const grid = document.createElement('div');
        grid.className = 'product-grid';
        
        categories[cat].forEach(p => {
            const isOutOfStock = p.stock <= 0;
            const inCart = cart.find(c => c.product_id === p.id);
            const qtyInCart = inCart ? inCart.qty : 0;
            const available = p.stock - qtyInCart;

            const card = document.createElement('div');
            card.className = 'product-card';
            card.innerHTML = `
                <img src="${p.image}" class="product-img" onerror="this.src='data:image/svg+xml;utf8,<svg xmlns=\\'http://www.w3.org/2000/svg\\' width=\\'100\\' height=\\'100\\'><rect width=\\'100\\' height=\\'100\\' fill=\\'%23eee\\'/><text x=\\'50%\\' y=\\'50%\\' dominant-baseline=\\'middle\\' text-anchor=\\'middle\\' fill=\\'%23999\\'>Sem Foto</text></svg>'">
                <div class="product-info">
                    <div class="product-name">${p.name}</div>
                    <div class="product-price">R$ ${p.price_sell.toFixed(2)}</div>
                </div>
                <button class="btn" ${available <= 0 ? 'disabled' : ''} onclick="addToCart('${p.id}')">
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

    if (item) {
        item.qty++;
    } else {
        cart.push({
            product_id: product.id,
            name: product.name,
            qty: 1,
            unit_price: product.price_sell
        });
    }
    
    updateCartUI();
    renderProducts(); // Re-render to update buttons
}

function updateCartUI() {
    const bar = document.getElementById('cart-bar');
    const countEl = document.getElementById('cart-count');
    const totalEl = document.getElementById('cart-total');

    if (cart.length === 0) {
        bar.style.display = 'none';
        return;
    }

    bar.style.display = 'flex';
    let total = 0;
    let count = 0;
    cart.forEach(item => {
        total += item.qty * item.unit_price;
        count += item.qty;
    });

    countEl.innerText = count;
    totalEl.innerText = `R$ ${total.toFixed(2)}`;
}

async function checkout() {
    if (cart.length === 0) return;
    
    let total = cart.reduce((acc, item) => acc + (item.qty * item.unit_price), 0);
    const payload = { items: cart, total: total };

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
            loadProducts(); // Reload to get fresh stock
        } else {
            alert('Erro ao finalizar compra.');
        }
    } catch (e) {
        alert('Erro de conexão.');
    }
}

// Init
loadProducts();
