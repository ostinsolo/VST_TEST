import React from 'react'
import ReactDOM from 'react-dom/client'
import Interface from './Interface.jsx'

import createHooks from 'zustand'
import createStore from 'zustand/vanilla'

import './index.css'

const store = createStore((set) => ({
  messages: [],
  currentUser: 'Ostin',
  setMessages: (newMessages) => set({ messages: newMessages }),
  addMessage: (message) => set(state => ({ messages: [...state.messages, message] })),
}));
const useStore = createHooks(store);

const errorStore = createStore(() => ({ error: null }));
const useErrorStore = createHooks(errorStore);

function sendMessage(message) {
  if (typeof globalThis.__sendMessage__ === 'function') {
    globalThis.__sendMessage__(JSON.stringify({
      message,
      username: store.getState().currentUser,
    }));
  }
}

if (process.env.NODE_ENV !== 'production') {
  import.meta.hot.on('reload-dsp', () => {
    console.log('Sending reload dsp message');
    if (typeof globalThis.__postNativeMessage__ === 'function') {
      globalThis.__postNativeMessage__('reload');
    }
  });
}

globalThis.__receiveStateChange__ = function(state) {
  const parsedState = JSON.parse(state);
  store.setState((prevState) => ({
    ...prevState,
    messages: parsedState.messages || prevState.messages,
    currentUser: parsedState.currentUser || prevState.currentUser,
  }));
};

globalThis.__receiveMessage__ = function(serializedMessage) {
  const message = JSON.parse(serializedMessage);
  store.getState().addMessage({
    sender: message.sender,
    text: message.text,
    timestamp: message.timestamp
  });
};

globalThis.__receiveError__ = (err) => {
  errorStore.setState({ error: err });
};

function App(props) {
  let state = useStore();
  let {error} = useErrorStore();

  return (
    <Interface
      {...state}
      error={error}
      sendMessage={sendMessage}
      setMessages={store.getState().setMessages}
      addMessage={store.getState().addMessage}
      resetErrorState={() => errorStore.setState({ error: null })} />
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
)

if (typeof globalThis.__postNativeMessage__ === 'function') {
  globalThis.__postNativeMessage__("ready");
}