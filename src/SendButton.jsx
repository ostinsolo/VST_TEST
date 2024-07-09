import React from 'react';

export default function SendButton({ onClick }) {
  return (
    <button
      className="bg-pink-500 hover:bg-pink-600 text-white font-bold py-2 px-4 rounded"
      onClick={onClick}
    >
      Send
    </button>
  );
}