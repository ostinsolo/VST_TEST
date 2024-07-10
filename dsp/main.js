// src/main.js
import axios from 'axios';

const API_BASE_URL = 'http://ableton-chat-01-72c15f63599a.herokuapp.com';
const API_SEND_ENDPOINT = `${API_BASE_URL}/messages/send`;
const API_GET_ENDPOINT = `${API_BASE_URL}/messages/get`;

let lastMessageTimestamp = 0;

async function sendMessageToAPI(nickname, message) {
  try {
    const response = await axios.post(API_SEND_ENDPOINT, { nickname, message });
    console.log('Message sent successfully');
    fetchNewMessages();
  } catch (error) {
    console.error('Error sending message:', error.response ? error.response.data : error.message);
  }
}

async function fetchNewMessages() {
  try {
    const response = await axios.post(API_GET_ENDPOINT, { fromTimestamp: lastMessageTimestamp });
    const messages = response.data.messages;
    if (messages.length > 0) {
      lastMessageTimestamp = messages[messages.length - 1].createdAt;
      messages.forEach(msg => {
        __postNativeMessage__('receiveMessage', JSON.stringify(msg));
      });
    }
  } catch (error) {
    console.error('Error fetching messages:', error.response ? error.response.data : error.message);
  }
}

// Handle sending messages from the UI
globalThis.__sendMessage__ = (serializedMessage) => {
  const { message, username } = JSON.parse(serializedMessage);
  sendMessageToAPI(username, message);
};

// Start fetching messages periodically
setInterval(fetchNewMessages, 10000);
