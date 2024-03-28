import { EVENT_RESET_DISPLAYS } from '../config.js';
import { post_redis_key_val, get_redis_val } from '../redis.js';

class Sai2InterfacesTabs extends HTMLElement {
	constructor() {
		super();
		Sai2InterfacesTabs.counter = (Sai2InterfacesTabs.counter || 0) + 1;
	}

	async connectedCallback() {
		const name = this.getAttribute('name').replace(/\s+/g, '_');
		this.uniqueId = name + Sai2InterfacesTabs.counter;
		const tabsContent = Array.from(this.children).filter(child => child.tagName === 'SAI2-INTERFACES-TAB-CONTENT');
		const otherContent = Array.from(this.children).filter(child => child.tagName != 'SAI2-INTERFACES-TAB-CONTENT');
		const position = this.getAttribute('tabsPosition') || 'top';
		const color = this.getAttribute('color') || 'rgb(0, 110, 255)';
		const key = this.getAttribute('key');

		// create map of tab values to names
		const tabValueToName = {};
		tabsContent.forEach((tabContent) => {
			const tabName = tabContent.getAttribute('name').replace(/\s+/g, '_');
			const tabValue = tabContent.getAttribute('value');
			tabValueToName[tabValue] = this.uniqueId + "_" + tabName + "-tab";
		});

		let activeTabName = sessionStorage.getItem(`${this.uniqueId}_activeTabName`);
		if (!activeTabName) {
			// Set the first tab as active if no active tab is stored
			const tabName = tabsContent[0].getAttribute('name').replace(/\s+/g, '_');
			activeTabName = this.uniqueId + "_" + tabName + "-tab";
		}

		if (key) {
			try {
				const value = await get_redis_val(key);
				if (value && tabValueToName[value]) {
					activeTabName = tabValueToName[value];
				}
			} catch (error) {
				console.error("Error occurred while getting value from Redis:", error);
			}
		}

		// Create the tabs HTML
		const tabsHTML = Array.from(tabsContent).map((tabContent, index) => {
			const tabNameDisplay = tabContent.getAttribute('name');
			const tabName = this.uniqueId + "_" + tabNameDisplay.replace(/\s+/g, '_') + "-tab";
			const isActive = tabName === activeTabName ? 'active' : '';
			const value = tabContent.getAttribute('value');
			return `<li class="nav-item">
                        <a class="nav-link ${isActive}" 
						id="${tabName}" data-bs-toggle="tab" 
						href="#${tabName}-content" role="tab" 
						aria-controls="${tabName}-content" 
						aria-selected="${isActive === 'active'}" 
						data-value="${value}">
							${tabNameDisplay}
						</a>
                    </li>`;
		}).join('');

		// Create the tab contents HTML
		const tabContentsHTML = Array.from(tabsContent).map((tabContent, index) => {
			const tabName = this.uniqueId + "_" + tabContent.getAttribute('name').replace(/\s+/g, '_') + "-tab";
			const isActive = tabName === activeTabName ? 'show active' : '';
			return `<div class="tab-pane fade ${isActive}" 
					id="${tabName}-content" role="tabpanel" 
					aria-labelledby="${tabName}">
                        ${tabContent.innerHTML}
                    </div>`;
		}).join('');

		// Construct the tabs structure
		const navType = position === 'left' ? 'nav-pills flex-column' : 'nav-tabs flex-row';
		const tabsClass = position === 'left' ? 'col-md-2' : 'row';
		const contentClass = position === 'left' ? 'col-md-10' : 'row';

		const borderClass = position === 'left' ? 'border-right' : 'border-bottom';
		const paddingClass = position === 'left' ? 'padding-right' : 'padding-top';

		this.innerHTML = `
		<style>
			.${this.uniqueId} .nav-link.active {
				background-color: ${color};
				color: rgb(240, 240, 240);
			}

			.${this.uniqueId} .nav-link {
				background-color: rgb(240, 240, 240);
				color: ${color};
			}

			.${this.uniqueId} {
				${borderClass}: 2px solid ${color};
				${paddingClass}: 10px;
			}
		</style>

		<div class="row">
			<div class="${tabsClass}">
				<ul class="nav ${navType} ${this.uniqueId}" id="${this.uniqueId}" role="tablist">
					${tabsHTML}
				</ul>
			</div>
			<div class="${contentClass}">
				<div class="tab-content" id="${this.uniqueId}-content">
					${tabContentsHTML}
				</div>
			</div>
		</div>
        `;

		// Add the other content to the tabs div
		let row = this.querySelector('.row');
		let tabsDiv = row.querySelector(`.${tabsClass}`);
		let ulDiv = tabsDiv.querySelector('ul');
		otherContent.forEach((content) => {
			ulDiv.appendChild(content);
		});

		// Add event listener to tabs for refreshing the page on tab switch 
		// and remembering which one is active
		this.querySelectorAll('.nav-link').forEach((tabLink) => {
			tabLink.addEventListener('show.bs.tab', () => {
				if (tabLink.getAttribute('id').startsWith(this.uniqueId)) {
					console.log('Tab clicked!', this.uniqueId, tabLink.getAttribute('id'));
					sessionStorage.setItem(`${this.uniqueId}_activeTabName`, tabLink.getAttribute('id'));

					const resetDisplaysEvent = new CustomEvent(EVENT_RESET_DISPLAYS, {
						bubbles: true,
						detail: { message: 'Reset the displays' }
					});
					document.dispatchEvent(resetDisplaysEvent);

					if (key) {
						const value = tabLink.getAttribute('data-value');
						if (value && value !== 'null') {
							post_redis_key_val(key, value);
						}
					}

				}
			});
		});

	}
}

class Sai2InterfacesTabContent extends HTMLElement {
	constructor() {
		super();
	}
}

customElements.define('sai2-interfaces-tabs', Sai2InterfacesTabs);
customElements.define('sai2-interfaces-tab-content', Sai2InterfacesTabContent);
