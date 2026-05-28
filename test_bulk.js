const fs = require('fs');

async function run() {
    const payload = [{
        name: "Test Product",
        category: "Test",
        price_cost: 10,
        price_sell: 20,
        stock: 5,
        active: true
    }];
    try {
        const res = await fetch('http://192.168.4.1/api/products/bulk', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(payload)
        });
        const text = await res.text();
        console.log("Status:", res.status);
        console.log("Body:", text);
    } catch(e) {
        console.error("Fetch failed:", e);
    }
}
run();
