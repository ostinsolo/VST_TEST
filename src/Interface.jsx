import React, { useState, useRef, useEffect } from 'react';
import { XCircleIcon, XMarkIcon } from '@heroicons/react/20/solid';

import SendButton from './SendButton';
import DragBar from './DragBar';
import MessageBox from './MessageBox';
import ChatHistory from './ChatHistory';

// Logo component (kept inline for simplicity, but you could move it to a separate file)
const Logo = (props) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    xmlSpace="preserve"
    fillRule="evenodd"
    clipRule="evenodd"
    strokeLinecap="round"
    strokeLinejoin="round"
    strokeMiterlimit={1.5}
    viewBox="0 0 2890 606"
    {...props}
  >
    
 
  </svg>
);

function ErrorAlert({ message, reset }) {
  return (
    <div className="rounded-md bg-red-50 p-4">
      <div className="flex">
        <div className="flex-shrink-0">
          <XCircleIcon className="h-5 w-5 text-red-400" aria-hidden="true" />
        </div>
        <div className="ml-3">
          <p className="text-sm font-medium text-red-800">{message}</p>
        </div>
        <div className="ml-auto pl-3">
          <div className="-mx-1.5 -my-1.5">
            <button
              type="button"
              onClick={reset}
              className="inline-flex rounded-md bg-red-50 p-1.5 text-red-500 hover:bg-red-100 focus:outline-none focus:ring-2 focus:ring-red-600 focus:ring-offset-2 focus:ring-offset-red-50"
            >
              <span className="sr-only">Dismiss</span>
              <XMarkIcon className="h-5 w-5" aria-hidden="true" />
            </button>
          </div>
        </div>
      </div>
    </div>
  )
}

export default function Interface(props) {
  const [chatWidth, setChatWidth] = useState(300); // Default width
  const chatHistoryRef = useRef(null);

  const handleSend = (message) => {
    props.sendMessage(message);

    // Immediately add the sent message to the UI
    props.addMessage({ sender: 'user', text: message, username: props.currentUser });
  };

  const handleResize = (newWidth) => {
    setChatWidth(newWidth);
  };

  useEffect(() => {
    if (chatHistoryRef.current) {
      chatHistoryRef.current.scrollTop = chatHistoryRef.current.scrollHeight;
    }
  }, [props.messages]);

  return (
    <div className="w-full h-screen min-w-[492px] min-h-[238px] bg-slate-800 flex flex-col overflow-hidden">
      <div className="h-1/5 flex justify-between items-center text-md text-slate-400 select-none p-8">
        <Logo className="h-8 w-auto text-slate-100" />
        <div>
          <span className="font-bold">HERE VST</span> &middot; {__BUILD_DATE__} 
        </div>
      </div>
      <div className="flex flex-1 overflow-hidden">
        <div className="flex-grow flex flex-col">
          {props.error && (<ErrorAlert message={props.error.message} reset={props.resetErrorState} />)}
          <div ref={chatHistoryRef} className="flex-grow overflow-y-auto">
            <ChatHistory messages={props.messages} />
          </div>
          <MessageBox onSend={handleSend} />
        </div>
        <DragBar onResize={handleResize} />
      </div>
    </div>
  );
}
