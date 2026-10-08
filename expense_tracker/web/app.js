// State
let allExpenses = [];
let currentStats = null;

const CATEGORY_COLORS = {
  'Food & Dining': '#d97706',
  'Transportation': '#0284c7',
  'Housing & Utilities': '#4f46e5',
  'Entertainment': '#db2777',
  'Healthcare': '#059669',
  'Shopping': '#9333ea',
  'Education': '#0891b2',
  'Personal Care': '#e11d48',
  'Travel': '#2563eb',
  'Other': '#475569'
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
  // Set default date to today
  const today = new Date().toISOString().split('T')[0];
  document.getElementById('date-input').value = today;

  // Event Listeners
  document.getElementById('add-expense-form').addEventListener('submit', handleAddExpense);
  document.getElementById('edit-expense-form').addEventListener('submit', handleSaveEdit);
  document.getElementById('category-select').addEventListener('change', handleCategorySelectChange);

  document.getElementById('search-input').addEventListener('input', renderTable);
  document.getElementById('filter-category').addEventListener('change', renderTable);
  document.getElementById('sort-select').addEventListener('change', renderTable);

  document.getElementById('modal-close-btn').addEventListener('click', closeModal);
  document.getElementById('modal-cancel-btn').addEventListener('click', closeModal);

  // Close modal on background click
  document.getElementById('edit-modal').addEventListener('click', (e) => {
    if (e.target.id === 'edit-modal') closeModal();
  });

  loadData();
});

// Toast notifications
function showToast(message, type = 'success') {
  const container = document.getElementById('toast-container');
  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `<span>${type === 'success' ? '✅' : '⚠️'}</span> <span>${message}</span>`;
  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(10px)';
    setTimeout(() => toast.remove(), 300);
  }, 3500);
}

// Custom category toggle
function handleCategorySelectChange(e) {
  const customGroup = document.getElementById('custom-cat-group');
  const customInput = document.getElementById('custom-cat-input');
  if (e.target.value === '__custom__') {
    customGroup.style.display = 'block';
    customInput.required = true;
    customInput.focus();
  } else {
    customGroup.style.display = 'none';
    customInput.required = false;
  }
}

// Load data from C Backend
async function loadData() {
  try {
    const [expensesRes, statsRes] = await Promise.all([
      fetch('/api/expenses'),
      fetch('/api/stats')
    ]);

    if (!expensesRes.ok || !statsRes.ok) {
      throw new Error('API server returned error');
    }

    allExpenses = await expensesRes.json();
    currentStats = await statsRes.json();

    renderKPIs(currentStats);
    renderCategoryBreakdown(currentStats.categories || []);
    renderTable();
  } catch (err) {
    console.error('Error fetching data:', err);
    showToast('Failed to connect to C backend.', 'error');
  }
}

// Render KPI Cards
function renderKPIs(stats) {
  if (!stats) return;

  document.getElementById('kpi-total').textContent = `₹${stats.total.toFixed(2)}`;
  document.getElementById('kpi-count').textContent = `${stats.count} recorded expense(s)`;
  document.getElementById('kpi-average').textContent = `₹${stats.average.toFixed(2)}`;

  if (stats.highest) {
    document.getElementById('kpi-highest').textContent = `₹${stats.highest.amount.toFixed(2)}`;
    document.getElementById('kpi-highest-meta').textContent = `${stats.highest.category} (${stats.highest.date})`;
  } else {
    document.getElementById('kpi-highest').textContent = '₹0.00';
    document.getElementById('kpi-highest-meta').textContent = 'None';
  }

  if (stats.lowest) {
    document.getElementById('kpi-lowest').textContent = `₹${stats.lowest.amount.toFixed(2)}`;
    document.getElementById('kpi-lowest-meta').textContent = `${stats.lowest.category} (${stats.lowest.date})`;
  } else {
    document.getElementById('kpi-lowest').textContent = '₹0.00';
    document.getElementById('kpi-lowest-meta').textContent = 'None';
  }
}

// Render Category Progress Bars
function renderCategoryBreakdown(categories) {
  const container = document.getElementById('category-bars');
  if (!categories || categories.length === 0) {
    container.innerHTML = '<p class="empty-hint" style="color: var(--text-dim); font-size: 0.85rem;">No category data yet.</p>';
    return;
  }

  // Sort by highest spending
  const sorted = [...categories].sort((a, b) => b.total - a.total);

  container.innerHTML = sorted.map(c => {
    const color = CATEGORY_COLORS[c.name] || '#6366f1';
    return `
      <div class="cat-bar-item">
        <div class="cat-bar-info">
          <span class="cat-bar-name" style="color: ${color}">● ${escapeHtml(c.name)}</span>
          <span class="cat-bar-amount">₹${c.total.toFixed(2)} (${c.percentage.toFixed(1)}%)</span>
        </div>
        <div class="progress-track">
          <div class="progress-fill" style="width: ${c.percentage}%; background: ${color};"></div>
        </div>
      </div>
    `;
  }).join('');
}

// Render Expense Table with Filters & Sorting
function renderTable() {
  const tbody = document.getElementById('expenses-tbody');
  const search = document.getElementById('search-input').value.trim().toLowerCase();
  const catFilter = document.getElementById('filter-category').value;
  const sortMode = document.getElementById('sort-select').value;

  // Filter
  let filtered = allExpenses.filter(e => {
    const matchSearch = !search ||
      e.description.toLowerCase().includes(search) ||
      e.category.toLowerCase().includes(search) ||
      e.date.includes(search);
    const matchCat = !catFilter || e.category === catFilter;
    return matchSearch && matchCat;
  });

  // Sort
  filtered.sort((a, b) => {
    if (sortMode === 'date-desc') return b.date.localeCompare(a.date);
    if (sortMode === 'date-asc') return a.date.localeCompare(b.date);
    if (sortMode === 'amount-desc') return b.amount - a.amount;
    if (sortMode === 'amount-asc') return a.amount - b.amount;
    return 0;
  });

  // Update badge
  document.getElementById('filtered-badge').textContent = `Showing ${filtered.length} of ${allExpenses.length}`;

  if (filtered.length === 0) {
    tbody.innerHTML = `
      <tr>
        <td colspan="6" class="empty-state">
          <p>🔍 No expenses found matching your criteria.</p>
        </td>
      </tr>
    `;
    return;
  }

  tbody.innerHTML = filtered.map(e => {
    const catColor = CATEGORY_COLORS[e.category] || 'var(--text-muted)';
    return `
      <tr>
        <td style="color: var(--text-dim); font-weight: 600;">#${e.id}</td>
        <td>${escapeHtml(e.date)}</td>
        <td>
          <span class="badge-category" style="border-color: ${catColor}; color: ${catColor}">
            ${escapeHtml(e.category)}
          </span>
        </td>
        <td style="font-weight: 500;">${escapeHtml(e.description)}</td>
        <td class="amount-cell">₹${e.amount.toFixed(2)}</td>
        <td>
          <div class="table-actions-cell">
            <button class="btn btn-secondary btn-sm" onclick="openEditModal(${e.id})">✏️ Edit</button>
            <button class="btn btn-danger btn-sm" onclick="handleDeleteExpense(${e.id})">🗑️ Delete</button>
          </div>
        </td>
      </tr>
    `;
  }).join('');
}

// Add Expense Handler
async function handleAddExpense(e) {
  e.preventDefault();

  const date = document.getElementById('date-input').value;
  const catSelect = document.getElementById('category-select').value;
  const customCat = document.getElementById('custom-cat-input').value.trim();
  const category = (catSelect === '__custom__') ? customCat : catSelect;
  const amount = parseFloat(document.getElementById('amount-input').value);
  const description = document.getElementById('desc-input').value.trim();

  if (!category) {
    showToast('Please specify a category.', 'error');
    return;
  }

  if (isNaN(amount) || amount <= 0) {
    showToast('Please enter a valid amount greater than 0.', 'error');
    return;
  }

  const payload = { date, category, amount, description };

  try {
    const res = await fetch('/api/expenses', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    if (res.ok) {
      showToast('Expense added successfully!', 'success');
      // Reset inputs (keep today's date)
      document.getElementById('amount-input').value = '';
      document.getElementById('desc-input').value = '';
      document.getElementById('custom-cat-input').value = '';
      document.getElementById('custom-cat-group').style.display = 'none';
      document.getElementById('category-select').value = 'Food & Dining';

      await loadData();
    } else {
      showToast('Failed to add expense.', 'error');
    }
  } catch (err) {
    console.error(err);
    showToast('Error communicating with backend.', 'error');
  }
}

// Open Edit Modal
window.openEditModal = function(id) {
  const expense = allExpenses.find(e => e.id === id);
  if (!expense) return;

  document.getElementById('edit-id').value = expense.id;
  document.getElementById('modal-expense-id').textContent = `#${expense.id}`;
  document.getElementById('edit-date').value = expense.date;
  document.getElementById('edit-amount').value = expense.amount;
  document.getElementById('edit-desc').value = expense.description;

  const catSelect = document.getElementById('edit-category');
  if ([...catSelect.options].some(o => o.value === expense.category)) {
    catSelect.value = expense.category;
  } else {
    // Add temporary option for custom category
    const opt = new Option(expense.category, expense.category, true, true);
    catSelect.add(opt);
  }

  document.getElementById('edit-modal').style.display = 'flex';
};

// Close Modal
function closeModal() {
  document.getElementById('edit-modal').style.display = 'none';
}

// Save Edit Handler
async function handleSaveEdit(e) {
  e.preventDefault();

  const id = parseInt(document.getElementById('edit-id').value, 10);
  const date = document.getElementById('edit-date').value;
  const category = document.getElementById('edit-category').value;
  const amount = parseFloat(document.getElementById('edit-amount').value);
  const description = document.getElementById('edit-desc').value.trim();

  const payload = { id, date, category, amount, description };

  try {
    const res = await fetch('/api/expenses/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    if (res.ok) {
      showToast('Expense updated successfully!', 'success');
      closeModal();
      await loadData();
    } else {
      showToast('Failed to update expense.', 'error');
    }
  } catch (err) {
    console.error(err);
    showToast('Error connecting to backend.', 'error');
  }
}

// Delete Handler
window.handleDeleteExpense = async function(id) {
  if (!confirm(`Are you sure you want to delete Expense #${id}?`)) {
    return;
  }

  try {
    const res = await fetch('/api/expenses/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id })
    });

    if (res.ok) {
      showToast(`Expense #${id} deleted.`, 'success');
      await loadData();
    } else {
      showToast('Failed to delete expense.', 'error');
    }
  } catch (err) {
    console.error(err);
    showToast('Error connecting to backend.', 'error');
  }
};

// Helper: Escape HTML strings
function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}
