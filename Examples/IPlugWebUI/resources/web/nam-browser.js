class NAMBrowser extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this.shadowRoot.innerHTML = `
      <style>
        .container {
          display: block;
          width: 100%;
          overflow-y: auto;
          max-height: 100%;
          font-size: 14px;
        }
        table {
          width: 100%;
          border-collapse: collapse;
        }
        th, td {
          padding: 8px;
          text-align: left;
          border-bottom: 1px solid #ddd;
        }
        th {
          position: sticky;
          top: 0;
          cursor: pointer;
        }
        tr {
          color: #fff;
          cursor: default;
        }
      </style>
      <div class="container">
        <table>
          <thead>
            <tr>
              <th>Name</th>
              <th>Author</th>
              <th>Date Created</th>
              <th>Import</th>
              <th>Download</th>
            </tr>
          </thead>
          <tbody>
            <!-- Table data will be dynamically populated here -->
          </tbody>
        </table>
      </div>
    `;
  }

  static get observedAttributes() {
    return ['data'];
  }

  attributeChangedCallback(name, oldValue, newValue) {
    if (name === 'data') {
      this.data = JSON.parse(newValue);
      this.renderData();
    }
  }

  renderData() {
    const tbody = this.shadowRoot.querySelector('tbody');
    tbody.innerHTML = '';

    for (const item of this.data) {
      const row = document.createElement('tr');
      row.innerHTML = `
        <td>${item.name}</td>
        <td>${item.author}</td>
        <td>${item.dateCreated}</td>
        <td>${item.import}</td>
        <td>${item.download}</td>
      `;
      tbody.appendChild(row);
    }
  }

  connectedCallback() {
    this.shadowRoot.querySelectorAll('th').forEach(header => {
      header.addEventListener('click', () => {
        this.sortTable(header.cellIndex);
      });
    });
  }

  sortTable(columnIndex) {
    const table = this.shadowRoot.querySelector('table');
    const rows = Array.from(table.rows)
      .slice(1)
      .sort((a, b) => a.cells[columnIndex].textContent.localeCompare(b.cells[columnIndex].textContent));

    for (const row of rows) {
      table.tBodies[0].appendChild(row);
    }
  }
}

customElements.define('nam-browser', NAMBrowser);
