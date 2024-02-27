class Sai2InterfacesTabs extends HTMLElement {
	constructor() {
		super();
	}

	connectedCallback() {
		const name = this.getAttribute('name').replace(/\s+/g, '_');
		const tabsContent = Array.from(this.children).filter(child => child.tagName === 'SAI2-INTERFACES-TAB-CONTENT');
		const position = this.getAttribute('tabsPosition') || 'top';
		const color = this.getAttribute('color') || 'rgb(0, 110, 255)';

		// Create the tabs HTML
		const tabsHTML = Array.from(tabsContent).map((tabContent, index) => {
			const tabNameDisplay = tabContent.getAttribute('name');
			const tabName = tabNameDisplay.replace(/\s+/g, '_');
			const isActive = index === 0 ? 'active' : '';
			return `<li class="nav-item">
                        <a class="nav-link ${isActive}" id="${name}_${tabName}-tab" data-bs-toggle="tab" href="#${name}_${tabName}" role="tab" aria-controls="${name}_${tabName}" aria-selected="${isActive === 'active'}">${tabNameDisplay}</a>
                    </li>`;
		}).join('');

		// Create the tab contents HTML
		const tabContentsHTML = Array.from(tabsContent).map((tabContent, index) => {
			const tabName = tabContent.getAttribute('name').replace(/\s+/g, '_');
			const isActive = index === 0 ? 'show active' : '';
			return `<div class="tab-pane fade ${isActive}" id="${name}_${tabName}" role="tabpanel" aria-labelledby="${name}_${tabName}-tab">
                        ${tabContent.innerHTML}
                    </div>`;
		}).join('');

		// Construct the tabs structure
		const navType = position === 'left' ? 'nav-pills flex-column' : 'nav-tabs flex-row';
		const tabsClass = position === 'left' ? 'col-md-2' : 'row';
		const contentClass = position === 'left' ? 'col-md-10' : 'row';
		this.innerHTML = `

		<style>
		.${name} .nav-link.active {
			background-color: ${color};
			color: rgb(240, 240, 240);
		}

		.${name} .nav-link {
			background-color: rgb(240, 240, 240);
			color: ${color};
		}
	</style>

		<div class="row">
                <div class="${tabsClass}">
                    <ul class="nav ${navType} ${name}" id="${name}" role="tablist">
                        ${tabsHTML}
                    </ul>
                </div>
                <div class="${contentClass}">
                    <div class="tab-content" id="${name}_content">
                        ${tabContentsHTML}
                    </div>
                </div>
            </div>
        `;
	}
}

class Sai2InterfacesTabContent extends HTMLElement {
	constructor() {
		super();
	}
}

customElements.define('sai2-interfaces-tabs', Sai2InterfacesTabs);
customElements.define('sai2-interfaces-tab-content', Sai2InterfacesTabContent);
