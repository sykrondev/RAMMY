import React, { useState, useEffect } from 'react';
import { useRammyLogic } from './useRammyLogic';
import GlassTheme from './themes/GlassTheme';
import FlanishyTheme from './themes/FlanishyTheme';

function App() {
  const logic = useRammyLogic();
  const [activeTheme, setActiveTheme] = useState('glass');

  // Load saved theme
  useEffect(() => {
    const saved = localStorage.getItem('rammyTheme') || 'glass';
    setActiveTheme(saved);
  }, []);

  const handleSetTheme = (theme: string) => {
    setActiveTheme(theme);
    localStorage.setItem('rammyTheme', theme);
  };

  if (activeTheme === 'glass') {
    return <GlassTheme logic={logic} activeTheme={activeTheme} setTheme={handleSetTheme} />;
  } else if (activeTheme === 'flanishy') {
    return <FlanishyTheme logic={logic} activeTheme={activeTheme} setTheme={handleSetTheme} />;
  }

  // Fallback
  return <GlassTheme logic={logic} activeTheme={'glass'} setTheme={handleSetTheme} />;
}

export default App;
