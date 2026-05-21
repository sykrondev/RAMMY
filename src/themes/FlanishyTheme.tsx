import React, { useState } from 'react';
import { RammyLogic } from '../useRammyLogic';
import { getCurrentWindow } from '@tauri-apps/api/window';
import bgImage from '../assets/flanishy_bg.png';

interface FlanishyThemeProps {
  logic: RammyLogic;
  activeTheme: string;
  setTheme: (theme: string) => void;
}

export default function FlanishyTheme({ logic, activeTheme, setTheme }: FlanishyThemeProps) {
  const [view, setView] = useState<'main' | 'settings'>('main');

  const {
    ramPct, ramUsed, ramTotal, isOptimizing, resultText,
    startupEnabled, toggleStartup,
    timerEnabled, setTimerEnabled, timerInterval, setTimerInterval, timeLeft,
    thresholdEnabled, setThresholdEnabled, ramThreshold, setRamThreshold,
    handleOptimize, handleClose, handleMinimize, language, setLanguage, t,
    closeToTray, setCloseToTray
  } = logic;

  const fontGothic = { fontFamily: '"Cinzel", serif' };
  const fontPixel = { fontFamily: '"VT323", monospace' };

  const renderMain = () => (
    <div className="flex-1 flex flex-col items-center justify-center p-3 gap-3 relative z-10" style={{ imageRendering: 'pixelated' }}>
      
      {/* Gothic Title Accent */}
      <div className="text-white drop-shadow-[2px_2px_0px_#000] tracking-[0.2em] text-lg bg-black/60 px-4 py-1 border-t-2 border-b-2 border-gray-400/50" style={fontGothic}>
        {t('seenAngel')}
      </div>

      <div className="relative w-32 h-48 flex items-center justify-center mb-0 mt-2">
        {/* Background Cross (Empty/Dark) */}
        <svg className="absolute inset-0 w-full h-full drop-shadow-[2px_2px_0px_#000]" viewBox="0 0 100 160" preserveAspectRatio="none">
          <path d="M 30 5 H 70 V 40 H 95 V 80 H 70 V 155 H 30 V 80 H 5 V 40 H 30 Z" fill="rgba(0,0,0,0.6)" stroke="#6b7280" strokeWidth="2" />
        </svg>

        {/* Foreground Cross (Filled from bottom) */}
        <svg className="absolute inset-0 w-full h-full" viewBox="0 0 100 160" preserveAspectRatio="none">
          <defs>
            <clipPath id="crossClip">
              <rect 
                x="0" 
                y={`${160 - (ramPct / 100) * 160}`} 
                width="100" 
                height={`${(ramPct / 100) * 160}`} 
                style={{ transition: 'y 0.8s cubic-bezier(0.4, 0, 0.2, 1), height 0.8s cubic-bezier(0.4, 0, 0.2, 1)' }}
              />
            </clipPath>
          </defs>
          <path 
            d="M 30 5 H 70 V 40 H 95 V 80 H 70 V 155 H 30 V 80 H 5 V 40 H 30 Z" 
            fill="#e5e7eb" 
            clipPath="url(#crossClip)"
          />
        </svg>

        {/* Text centered at the intersection of the cross */}
        <div className="flex flex-col items-center z-10 text-white drop-shadow-[2px_2px_0px_#000] absolute inset-0 justify-start pt-[50px]">
          <span className="text-4xl font-black leading-none" style={fontPixel}>{ramPct}</span>
          <span className="text-[10px] mt-1 tracking-widest text-gray-300 font-bold" style={fontGothic}>% USED</span>
        </div>
      </div>

      <div className="flex w-full justify-around bg-black/80 border-2 border-gray-400 p-2 mb-0 shadow-[2px_2px_0px_#000] relative overflow-hidden">
        <div className="absolute top-0 left-0 w-full h-full bg-[url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0IiBoZWlnaHQ9IjQiPgo8cmVjdCB3aWR0aD0iNCIgaGVpZ2h0PSI0IiBmaWxsPSIjZmZmIiBmaWxsLW9wYWNpdHk9IjAuMDUiLz4KPC9zdmc+')] opacity-40 pointer-events-none"></div>
        <div className="flex flex-col items-center relative z-10 px-2">
          <span className="text-xs text-gray-300 mb-0 tracking-widest uppercase font-bold" style={fontGothic}>{t('used')}</span>
          <span className="text-xl text-white drop-shadow-[2px_2px_0px_#000]" style={fontPixel}>{ramUsed}</span>
        </div>
        <div className="w-px bg-gray-500"></div>
        <div className="flex flex-col items-center relative z-10 px-2">
          <span className="text-xs text-gray-300 mb-0 tracking-widest uppercase font-bold" style={fontGothic}>{t('total')}</span>
          <span className="text-xl text-white drop-shadow-[2px_2px_0px_#000]" style={fontPixel}>{ramTotal}</span>
        </div>
      </div>

      {resultText ? (
        <div className="w-full text-center text-lg text-black bg-gray-300 border-2 border-black py-2 shadow-[2px_2px_0px_#000]" style={fontGothic}>
          {resultText}
        </div>
      ) : (
        <button 
          onClick={() => handleOptimize()}
          disabled={isOptimizing}
          className="w-full py-2 bg-gray-200 border-4 border-gray-400 border-b-gray-600 border-r-gray-600 text-black text-xl font-bold shadow-[2px_2px_0px_#000] hover:bg-white active:border-t-gray-600 active:border-l-gray-600 active:border-b-gray-400 active:border-r-gray-400 transition-none disabled:opacity-50 relative overflow-hidden group"
          style={fontGothic}
        >
          <div className="absolute inset-0 bg-gradient-to-r from-transparent via-white/50 to-transparent translate-x-[-100%] group-hover:animate-[shimmer_1s_infinite]"></div>
          † {t('optimize')} †
        </button>
      )}

      {timerEnabled && !isOptimizing && (
        <div className="text-lg text-white drop-shadow-[2px_2px_0px_#000] mt-0 bg-black/60 px-2 py-0.5 border border-gray-500" style={fontPixel}>
          {t('autoCleanIn')} {Math.floor(timeLeft / 60)}:{(timeLeft % 60).toString().padStart(2, '0')}
        </div>
      )}
    </div>
  );

  const renderSettings = () => (
    <div className="flex-1 flex flex-col p-4 gap-4 relative z-10 bg-gray-900/90 text-white border-t-2 border-gray-400 overflow-y-auto custom-scrollbar-gothic" style={{ imageRendering: 'pixelated' }}>
      
      <div className="text-center text-3xl mb-4 text-white drop-shadow-[0_2px_4px_#000]" style={fontGothic}>{t('options')}</div>

      {/* Language Setting */}
      <div className="flex flex-col bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000]">
        <div className="flex justify-between items-center mb-1">
          <span className="font-bold tracking-wider uppercase">{t('language')}</span>
          <select 
            value={language}
            onChange={(e) => setLanguage(e.target.value)}
            className="bg-gray-300 text-black font-bold p-1 outline-none cursor-pointer border-2 border-b-gray-500 border-r-gray-500 border-t-gray-100 border-l-gray-100"
          >
            <option value="en">English</option>
            <option value="es">Español</option>
          </select>
        </div>
      </div>

      {/* Theme Switcher */}
      <div className="flex flex-col bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000]">
        <div className="flex justify-between items-center mb-1">
          <span className="font-bold tracking-wider uppercase">{t('uiTheme')}</span>
        <select 
          value={activeTheme} 
          onChange={(e) => setTheme(e.target.value)}
          className="bg-gray-900 border border-gray-400 text-white px-2 py-1 outline-none"
        >
          <option value="glass">Glass</option>
          <option value="flanishy">Flanishy</option>
        </select>
        </div>
      </div>

      {/* Startup Setting */}
      <div className="flex justify-between items-center bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000] cursor-pointer hover:bg-black/70" onClick={toggleStartup}>
        <span className="font-bold tracking-wider uppercase">{t('startup')}</span>
        <div className={`w-6 h-6 border-2 flex items-center justify-center ${startupEnabled ? 'bg-white border-white text-black' : 'border-gray-500 text-transparent'}`} style={fontPixel}>✓</div>
      </div>

      {/* Close To Tray Setting */}
      <div className="flex justify-between items-center bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000] cursor-pointer hover:bg-black/70" onClick={() => setCloseToTray(!closeToTray)}>
        <span className="font-bold tracking-wider uppercase">{t('closeToTray')}</span>
        <div className={`w-6 h-6 border-2 flex items-center justify-center ${closeToTray ? 'bg-white border-white text-black' : 'border-gray-500 text-transparent'}`} style={fontPixel}>✓</div>
      </div>

      {/* Threshold Setting */}
      <div className="flex flex-col bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000]">
        <div className="flex justify-between items-center mb-2 cursor-pointer" onClick={() => setThresholdEnabled(!thresholdEnabled)}>
          <span className="font-bold tracking-wider uppercase">{t('autoCleanRam')}</span>
          <div className={`w-6 h-6 border-2 flex items-center justify-center ${thresholdEnabled ? 'bg-white border-white text-black' : 'border-gray-500 text-transparent'}`} style={fontPixel}>✓</div>
        </div>
        <div className={`flex items-center gap-4 transition-opacity ${thresholdEnabled ? 'opacity-100' : 'opacity-40 pointer-events-none'}`}>
          <input 
            type="range" min="1" max="100" step="1"
            value={ramThreshold} 
            onChange={(e) => setRamThreshold(parseInt(e.target.value))}
            className="flex-1 accent-white"
          />
          <span className="w-10 text-right text-white drop-shadow-[0_0_5px_#fff] text-xl" style={fontPixel}>{ramThreshold}%</span>
        </div>
      </div>

      {/* Timer Setting */}
      <div className="flex flex-col bg-black/50 p-2 border-2 border-gray-400 shadow-[2px_2px_0px_#000]">
        <div className="flex justify-between items-center mb-2 cursor-pointer" onClick={() => setTimerEnabled(!timerEnabled)}>
          <span className="font-bold tracking-wider uppercase">{t('autoCleanTimer')}</span>
          <div className={`w-6 h-6 border-2 flex items-center justify-center ${timerEnabled ? 'bg-white border-white text-black' : 'border-gray-500 text-transparent'}`} style={fontPixel}>✓</div>
        </div>
        <div className={`flex items-center gap-4 transition-opacity ${timerEnabled ? 'opacity-100' : 'opacity-40 pointer-events-none'}`}>
          <input 
            type="range" min="1" max="200" step="1"
            value={timerInterval} 
            onChange={(e) => setTimerInterval(parseInt(e.target.value))}
            className="flex-1 accent-white"
          />
          <span className="w-10 text-right text-white drop-shadow-[0_0_5px_#fff] text-xl" style={fontPixel}>{timerInterval}</span>
        </div>
      </div>

    </div>
  );

  return (
    <div 
      className="w-full h-screen flex flex-col relative overflow-hidden bg-gray-500"
      style={{
        backgroundImage: `url(${bgImage})`,
        backgroundSize: 'cover',
        backgroundPosition: 'center',
        backgroundBlendMode: 'multiply' // Re-added subtle dark filter by blending with gray-500
      }}
    >
      {/* PSX Dither/Scanline Overlay */}
      <div className="absolute inset-0 opacity-30 pointer-events-none z-0" style={{ 
        backgroundImage: `url('data:image/svg+xml;utf8,<svg width="4" height="4" xmlns="http://www.w3.org/2000/svg"><rect width="2" height="2" fill="black"/><rect x="2" y="2" width="2" height="2" fill="black"/></svg>')`,
        backgroundRepeat: 'repeat' 
      }}></div>
      
      {/* Dark vignette over the edges for contrast but keeping center bright */}
      <div className="absolute inset-0 shadow-[inset_0_0_80px_rgba(0,0,0,0.9)] pointer-events-none z-0"></div>
      
      {/* Titlebar */}
      <div 
        onMouseDown={() => getCurrentWindow().startDragging()}
        className="h-8 flex justify-between items-center px-2 bg-gray-800 border-b-2 border-gray-400 cursor-move select-none relative z-20 shadow-[0_2px_0_rgba(0,0,0,1)]"
        style={{ imageRendering: 'pixelated' }}
      >
        <div className="flex items-center gap-2 pointer-events-none text-white drop-shadow-[1px_1px_0_#000]">
          <span className="font-bold text-lg tracking-[0.1em]" style={fontGothic}>RAMMY</span>
        </div>
        <div className="flex gap-2 items-center" onMouseDown={(e) => e.stopPropagation()}>
          <button onClick={() => setView(view === 'main' ? 'settings' : 'main')} className="w-6 h-6 bg-gray-300 border-2 border-gray-100 border-b-gray-500 border-r-gray-500 text-black flex items-center justify-center hover:bg-white active:border-t-gray-500 active:border-l-gray-500 active:border-b-gray-100 active:border-r-gray-100" title="Settings" style={fontPixel}>
            {view === 'main' ? '⚙' : '⟵'}
          </button>
          <button onClick={handleMinimize} className="w-6 h-6 bg-gray-300 border-2 border-gray-100 border-b-gray-500 border-r-gray-500 text-black flex items-center justify-center hover:bg-white active:border-t-gray-500 active:border-l-gray-500 active:border-b-gray-100 active:border-r-gray-100 leading-none" style={fontPixel}>_</button>
          <button onClick={handleClose} className="w-6 h-6 bg-red-400 border-2 border-red-200 border-b-red-600 border-r-red-600 text-black flex items-center justify-center hover:bg-red-300 active:border-t-red-600 active:border-l-red-600 active:border-b-red-200 active:border-r-red-200 font-bold" style={fontPixel}>X</button>
        </div>
      </div>

      {/* Content Area */}
      {view === 'main' ? renderMain() : renderSettings()}

    </div>
  );
}
