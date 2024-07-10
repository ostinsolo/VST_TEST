// Importing axios to handle HTTP requests
import axios from 'axios';

// Setting up API endpoints
const API_BASE_URL = 'http://ableton-chat-01-72c15f63599a.herokuapp.com';
const API_SEND_ENDPOINT = `${API_BASE_URL}/messages/send`;
const API_GET_ENDPOINT = `${API_BASE_URL}/messages/get`;

// State variables
let lastMessageTimestamp = 0;

// Function to send messages to the API
async function sendMessageToAPI(nickname, message) {
  try {
    const response = await axios.post(API_SEND_ENDPOINT, { nickname, message });
    console.log('Message sent successfully');
    fetchNewMessages();  // Fetch new messages after sending to ensure UI is updated
  } catch (error) {
    console.error('Error sending message:', error.response ? error.response.data : error.message);
  }
}

// Function to fetch new messages from the API
async function fetchNewMessages() {
  console.log("Attempting to fetch new messages...");
  try {
    const response = await axios.post(API_GET_ENDPOINT, { fromTimestamp: lastMessageTimestamp });
    const messages = response.data.messages;
    console.log(`Fetched ${messages.length} new messages.`);
    if (messages.length > 0) {
      lastMessageTimestamp = messages[messages.length - 1].createdAt; // Update the timestamp
      messages.forEach(msg => {
        __postNativeMessage__('receiveMessage', JSON.stringify(msg)); // Post each message to the native interface
      });
    }
  } catch (error) {
    console.error('Error fetching messages:', error.response ? error.response.data : error.message);
    console.log("Retrying fetch in 10 seconds...");
    setTimeout(fetchNewMessages, 10000); // Retry fetching after a delay on error
  }
}

// Function to handle sending messages from the UI, tying UI actions to backend operations
globalThis.__sendMessage__ = (serializedMessage) => {
  const { message, username } = JSON.parse(serializedMessage);
  sendMessageToAPI(username, message);
};

// Set an interval to fetch new messages periodically, ensuring new data is regularly fetched
setInterval(fetchNewMessages, 10000);
