import React from 'react';
import DragBehavior from './DragBehavior';

export default function DragBar({ onResize }) {
  return (
    <DragBehavior
      className="w-2 bg-slate-600 hover:bg-slate-500 cursor-col-resize"
      onChange={(value) => onResize(value * 300)} // Adjust the multiplier as needed
    />
  );
}