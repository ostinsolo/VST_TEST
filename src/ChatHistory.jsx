import React from 'react';

export default function ChatHistory({ messages }) {
    return (
      <div className="p-4 space-y-4">
        {messages.map((msg, index) => (
          <div key={index} className={`p-2 rounded ${msg.sender === 'user' ? 'bg-slate-200 ml-auto' : 'bg-pink-200'} max-w-3/4`}>
            <div className="font-bold">{msg.username}</div>
            <div>{msg.text}</div>
          </div>
        ))}
      </div>
    );
  }