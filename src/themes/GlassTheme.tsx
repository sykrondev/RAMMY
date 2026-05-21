import React, { useState } from 'react';
import { GlassOpacityMode, RammyLogic } from '../useRammyLogic';
import { getCurrentWindow } from '@tauri-apps/api/window';

interface GlassThemeProps {
  logic: RammyLogic;
  activeTheme: string;
  setTheme: (theme: string) => void;
}

export default function GlassTheme({ logic, activeTheme, setTheme }: GlassThemeProps) {
  const [view, setView] = useState<'main' | 'settings'>('main');

  const {
    ramPct, ramUsed, ramTotal, isOptimizing, resultText,
    startupEnabled, toggleStartup,
    timerEnabled, setTimerEnabled, timerInterval, setTimerInterval, timeLeft,
    thresholdEnabled, setThresholdEnabled, ramThreshold, setRamThreshold,
    handleOptimize, handleClose, handleMinimize, language, setLanguage, t,
    closeToTray, setCloseToTray,
    glassOpacityMode, setGlassOpacityMode
  } = logic;

  const opacityModes: GlassOpacityMode[] = ['transparent', 'medium', 'solid'];
  const opacityValue = opacityModes.indexOf(glassOpacityMode);

  const renderMain = () => (
    <div className="flex-1 flex flex-col items-center justify-center p-6 gap-4 animate-fade-in">
      <div className="relative w-40 h-40 flex items-center justify-center mb-2">
        <div className="absolute inset-0 rounded-full border-4 border-blue-900/40"></div>
        <div className="absolute inset-2 rounded-full border border-blue-400/20 shadow-[inset_0_0_20px_rgba(96,165,250,0.1)]"></div>
        
        <svg className="absolute inset-0 w-full h-full transform -rotate-90 filter drop-shadow-[0_0_8px_rgba(96,165,250,0.5)]">
          <circle cx="80" cy="80" r="70" fill="transparent" stroke="rgba(96, 165, 250, 0.15)" strokeWidth="8" />
          <circle cx="80" cy="80" r="70" fill="transparent" stroke="url(#blue-gradient)" strokeWidth="8" 
            strokeDasharray={`${2 * Math.PI * 70}`}
            strokeDashoffset={`${2 * Math.PI * 70 * (1 - ramPct / 100)}`}
            strokeLinecap="round"
            style={{ transition: 'stroke-dashoffset 0.8s cubic-bezier(0.4, 0, 0.2, 1)' }}
          />
          <defs>
            <linearGradient id="blue-gradient" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stopColor="#93c5fd" />
              <stop offset="100%" stopColor="#2563eb" />
            </linearGradient>
          </defs>
        </svg>

        <div className="flex flex-col items-center z-10">
          <span className="text-4xl font-black chrome-text tabular-nums">{ramPct}</span>
          <span className="text-[10px] text-blue-300 font-bold uppercase tracking-widest mt-1 opacity-80">% USED</span>
        </div>
      </div>

      <div className="flex w-full justify-around aero-glass rounded-xl p-3 mb-2 shadow-inner">
        <div className="flex flex-col items-center">
          <span className="text-[9px] text-blue-300/80 uppercase tracking-widest mb-1">{t('used')}</span>
          <span className="font-bold text-sm text-blue-50 drop-shadow-md">{ramUsed}</span>
        </div>
        <div className="w-px bg-gradient-to-b from-transparent via-blue-400/30 to-transparent"></div>
        <div className="flex flex-col items-center">
          <span className="text-[9px] text-blue-300/80 uppercase tracking-widest mb-1">{t('total')}</span>
          <span className="font-bold text-sm text-blue-50 drop-shadow-md">{ramTotal}</span>
        </div>
      </div>

      {resultText ? (
        <div className="w-full text-center text-xs font-medium text-blue-200 animate-pulse bg-blue-900/30 rounded py-2 border border-blue-400/20">
          {resultText}
        </div>
      ) : (
        <button 
          onClick={() => handleOptimize()}
          disabled={isOptimizing}
          className="w-full py-3 bg-blue-500/80 rounded-lg text-white font-bold tracking-widest shadow-lg glow-btn border border-blue-400/50 hover:bg-blue-400/90 transition-all disabled:opacity-50"
        >
          {t('optimize')}
        </button>
      )}

      {timerEnabled && !isOptimizing && (
        <div className="text-sm text-blue-300/80 mt-2 font-mono tracking-widest font-light">
          {t('autoCleanIn')} {Math.floor(timeLeft / 60)}:{(timeLeft % 60).toString().padStart(2, '0')}
        </div>
      )}
    </div>
  );

  const renderSettings = () => (
    <div className="flex-1 flex flex-col p-6 gap-6 relative z-10 aero-glass border-t border-white/20 overflow-y-auto custom-scrollbar">
      
      <div className="text-center text-2xl mb-2 font-light tracking-widest text-blue-200">{t('options')}</div>

      {/* Language Switcher */}
      <div className="flex justify-between items-center border-b border-white/10 pb-4">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('language')}</span>
        <select 
          value={language} 
          onChange={(e) => setLanguage(e.target.value)}
          className="bg-black/50 border border-white/20 text-white px-3 py-1 rounded outline-none"
        >
          <option value="en">English</option>
          <option value="es">Español</option>
        </select>
      </div>

      {/* Theme Switcher */}
      <div className="flex justify-between items-center border-b border-white/10 pb-4">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('uiTheme')}</span>
        <select 
          value={activeTheme} 
          onChange={(e) => setTheme(e.target.value)}
          className="bg-black/50 border border-white/20 text-white px-3 py-1 rounded outline-none"
        >
          <option value="glass">Glass</option>
          <option value="flanishy">Flanishy</option>
        </select>
      </div>

      <div className="flex flex-col gap-2 border-b border-white/10 pb-4">
        <div className="flex justify-between items-center">
          <span className="text-sm tracking-wider text-gray-300 uppercase">{t('glassTransparency')}</span>
          <span className="text-xs text-blue-200 uppercase tracking-widest">{t(glassOpacityMode)}</span>
        </div>
        <input
          type="range"
          min="0"
          max="2"
          step="1"
          value={opacityValue}
          onChange={(e) => setGlassOpacityMode(opacityModes[parseInt(e.target.value)])}
          className="w-full accent-blue-400"
        />
        <div className="flex justify-between text-[9px] text-blue-300/70 uppercase tracking-widest">
          <span>{t('transparent')}</span>
          <span>{t('medium')}</span>
          <span>{t('solid')}</span>
        </div>
      </div>

      <div className="flex justify-between items-center border-b border-white/10 pb-4">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('startup')}</span>
        <button onClick={toggleStartup} className={`w-12 h-6 rounded-full transition-colors relative ${startupEnabled ? 'bg-blue-500' : 'bg-gray-600'}`}>
          <div className={`w-5 h-5 rounded-full bg-white absolute top-0.5 transition-all ${startupEnabled ? 'left-6' : 'left-0.5'}`}></div>
        </button>
      </div>

      <div className="flex justify-between items-center border-b border-white/10 pb-4">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('closeToTray')}</span>
        <button onClick={() => setCloseToTray(!closeToTray)} className={`w-12 h-6 rounded-full transition-colors relative ${closeToTray ? 'bg-blue-500' : 'bg-gray-600'}`}>
          <div className={`w-5 h-5 rounded-full bg-white absolute top-0.5 transition-all ${closeToTray ? 'left-6' : 'left-0.5'}`}></div>
        </button>
      </div>

      <div className="flex justify-between items-center border-b border-white/10 pb-4">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('autoCleanTimer')}</span>
        <button onClick={() => setTimerEnabled(!timerEnabled)} className={`w-12 h-6 rounded-full transition-colors relative ${timerEnabled ? 'bg-blue-500' : 'bg-gray-600'}`}>
          <div className={`w-5 h-5 rounded-full bg-white absolute top-0.5 transition-all ${timerEnabled ? 'left-6' : 'left-0.5'}`}></div>
        </button>
      </div>

      <div className={`flex flex-col gap-2 transition-opacity ${timerEnabled ? 'opacity-100' : 'opacity-40 pointer-events-none'}`}>
        <div className="flex justify-between items-center">
          <span className="text-xs text-gray-400 uppercase tracking-widest">Interval (Mins)</span>
          <span className="text-xl font-light text-blue-200">{timerInterval}</span>
        </div>
        <input type="range" min="1" max="200" step="1" value={timerInterval} onChange={e => setTimerInterval(parseInt(e.target.value))} className="w-full accent-blue-500" />
      </div>

      <div className="flex justify-between items-center border-b border-white/10 pb-4 mt-2">
        <span className="text-sm tracking-wider text-gray-300 uppercase">{t('autoCleanRam')}</span>
        <button onClick={() => setThresholdEnabled(!thresholdEnabled)} className={`w-12 h-6 rounded-full transition-colors relative ${thresholdEnabled ? 'bg-blue-500' : 'bg-gray-600'}`}>
          <div className={`w-5 h-5 rounded-full bg-white absolute top-0.5 transition-all ${thresholdEnabled ? 'left-6' : 'left-0.5'}`}></div>
        </button>
      </div>

      <div className={`flex flex-col gap-2 pb-6 transition-opacity ${thresholdEnabled ? 'opacity-100' : 'opacity-40 pointer-events-none'}`}>
        <div className="flex justify-between items-center">
          <span className="text-xs text-gray-400 uppercase tracking-widest">{t('autoCleanRam')}</span>
          <span className="font-bold">{ramThreshold}%</span>
        </div>
        <input type="range" min="1" max="100" step="1" value={ramThreshold} onChange={e => setRamThreshold(parseInt(e.target.value))} className="w-full accent-blue-400" />
      </div>

    </div>
  );

  return (
    <div className={`w-full h-screen rounded-2xl border border-blue-400/40 radial-bg glass-opacity-${glassOpacityMode} flex flex-col relative overflow-hidden shadow-[0_0_30px_rgba(30,58,138,0.6)]`}>
      {/* Titlebar */}
      <div 
        onMouseDown={() => getCurrentWindow().startDragging()}
        className="h-12 flex justify-between items-center px-4 bg-gradient-to-b from-white/10 to-transparent border-b border-white/10 backdrop-blur-md cursor-move select-none"
      >
        <div className="flex items-center gap-2 pointer-events-none">
          <div className="w-2.5 h-2.5 rounded-full bg-blue-400 shadow-[0_0_10px_#60a5fa] animate-pulse"></div>
          <span className="font-black tracking-[0.2em] text-xs text-blue-50 uppercase drop-shadow">RAMMY</span>
        </div>
        <div className="flex gap-4 items-center" onMouseDown={(e) => e.stopPropagation()}>
          <button onClick={() => setView(view === 'main' ? 'settings' : 'main')} className="text-blue-300 hover:text-white transition-colors text-lg" title="Settings">
            {view === 'main' ? '⚙' : '⟵'}
          </button>
          <button onClick={handleMinimize} className="text-blue-300 hover:text-white transition-colors text-xl leading-none">−</button>
          <button onClick={handleClose} className="text-blue-300 hover:text-red-400 transition-colors text-sm font-bold">✕</button>
        </div>
      </div>

      {/* Content Area */}
      {view === 'main' ? renderMain() : renderSettings()}
    </div>
  );
}
