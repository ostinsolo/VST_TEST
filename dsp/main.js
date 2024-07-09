// src/main.js
const API_BASE_URL = 'http://ableton-chat-01-72c15f63599a.herokuapp.com';
const API_SEND_ENDPOINT = `${API_BASE_URL}/messages/send`;
const API_GET_ENDPOINT = `${API_BASE_URL}/messages/get`;

let lastMessageTimestamp = 0;

async function sendMessageToAPI(nickname, message) {
  try {
    const response = await fetch(API_SEND_ENDPOINT, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ nickname, message }),
    });
    if (!response.ok) throw new Error('Failed to send message');
    console.log('Message sent successfully');
    fetchNewMessages();
  } catch (error) {
    console.error('Error sending message:', error);
  }
}

async function fetchNewMessages() {
  try {
    const response = await fetch(API_GET_ENDPOINT, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ fromTimestamp: lastMessageTimestamp }),
    });
    if (!response.ok) throw new Error('Failed to fetch messages');
    const data = await response.json();
    const messages = data.messages;
    if (messages.length > 0) {
      lastMessageTimestamp = messages[messages.length - 1].createdAt;
      messages.forEach(msg => {
        __postNativeMessage__('receiveMessage', JSON.stringify(msg));
      });
    }
  } catch (error) {
    console.error('Error fetching messages:', error);
  }
}

// Handle sending messages from the UI
globalThis.__sendMessage__ = (serializedMessage) => {
  const { message, username } = JSON.parse(serializedMessage);
  sendMessageToAPI(username, message);
};

// Start fetching messages periodically
setInterval(fetchNewMessages, 10000);
