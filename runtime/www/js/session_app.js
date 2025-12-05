document.addEventListener('DOMContentLoaded', () => {

    const apiEndpoint = '/cgi-bin/session_logic.py';

    // --- DOM Elements ---
    const themeToggle = document.getElementById('theme-toggle-checkbox');
    const userDisplay = document.getElementById('user-display');
    const sessionDataDisplay = document.getElementById('session-data-display');
    const loginForm = document.getElementById('login-form');
    const logoutButton = document.getElementById('logout-button');
    const customVarForm = document.getElementById('custom-var-form');
    const usernameInput = document.getElementById('username');

    /**
     * Fetches data from the CGI backend.
     * @param {URLSearchParams} params - The query parameters for the request.
     * @returns {Promise<object>} - A promise that resolves with the session data.
     */
    const fetchSessionState = async (params = new URLSearchParams('action=getState')) => {
        try {
            const response = await fetch(`${apiEndpoint}?${params.toString()}`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return await response.json();
        } catch (error) {
            console.error("Error fetching session state:", error);
            // Display error on page
            sessionDataDisplay.innerHTML = `<p style="color: red;">Error communicating with server.</p>`;
            return {};
        }
    };

    /**
     * Updates the entire UI based on the session data provided.
     * @param {object} state - The session data object.
     */
    const updateUI = (state) => {
        // Update theme
        const isDark = state.theme === 'dark';
        themeToggle.checked = isDark;
        document.body.classList.toggle('dark-theme', isDark);
        document.body.classList.toggle('light-theme', !isDark);

        // Update user display and form visibility
        const user = state.userid || 'Guest';
        userDisplay.textContent = user;
        if (user !== 'Guest') {
            loginForm.style.display = 'none';
            logoutButton.style.display = 'block';
        } else {
            loginForm.style.display = 'block';
            logoutButton.style.display = 'none';
        }

        // Update session data display
        sessionDataDisplay.innerHTML = ''; // Clear previous data
        const dataList = document.createElement('ul');
        for (const [key, value] of Object.entries(state)) {
            const listItem = document.createElement('li');
            listItem.innerHTML = `<strong>${key}:</strong> ${value}`;
            dataList.appendChild(listItem);
        }
        sessionDataDisplay.appendChild(dataList);
    };

    // --- Event Listeners ---

    themeToggle.addEventListener('change', async () => {
        const newTheme = themeToggle.checked ? 'dark' : 'light';
        const params = new URLSearchParams({ action: 'setTheme', theme: newTheme });
        const newState = await fetchSessionState(params);
        updateUI(newState);
    });

    loginForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = usernameInput.value.trim();
        if (username) {
            const params = new URLSearchParams({ action: 'login', username });
            const newState = await fetchSessionState(params);
            updateUI(newState);
        }
    });

    logoutButton.addEventListener('click', async () => {
        const params = new URLSearchParams({ action: 'logout' });
        const newState = await fetchSessionState(params);
        updateUI(newState);
        usernameInput.value = ''; // Clear username field on logout
    });

    customVarForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const formData = new FormData(e.target);
        const key = formData.get('key').trim();
        const value = formData.get('value').trim();
        if (key && value !== '') {
            const params = new URLSearchParams({ action: 'setCustom', key, value });
            const newState = await fetchSessionState(params);
            updateUI(newState);
            e.target.reset(); // Clear form fields
        }
    });

    // --- Initial Load ---
    const init = async () => {
        const initialState = await fetchSessionState();
        updateUI(initialState);
    };

    init();
});
