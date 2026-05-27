import React, { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { isPermissionGranted, requestPermission, sendNotification } from '@tauri-apps/plugin-notification';

export type GlassOpacityMode = 'transparent' | 'medium' | 'solid';

export interface RammyLogic {
  ramPct: number;
  ramUsed: string;
  ramTotal: string;
  isOptimizing: boolean;
  resultText: string;
  
  startupEnabled: boolean;
  toggleStartup: () => void;
  
  timerEnabled: boolean;
  setTimerEnabled: (val: boolean) => void;
  timerInterval: number;
  setTimerInterval: (val: number) => void;
  timeLeft: number;
  
  thresholdEnabled: boolean;
  setThresholdEnabled: (val: boolean) => void;
  ramThreshold: number;
  setRamThreshold: (val: number) => void;
  
  handleOptimize: (auto?: boolean) => void;
  handleClose: () => void;
  handleMinimize: () => void;
  
  language: string;
  setLanguage: (lang: string) => void;
  t: (key: string) => string;

  closeToTray: boolean;
  setCloseToTray: (val: boolean) => void;

  glassOpacityMode: GlassOpacityMode;
  setGlassOpacityMode: (val: GlassOpacityMode) => void;
}

const translations: Record<string, Record<string, string>> = {
  en: {
    options: "Options",
    uiTheme: "UI Theme",
    language: "Language",
    startup: "Run on Startup",
    autoCleanRam: "Auto-clean if RAM >",
    autoCleanTimer: "Auto-clean every",
    used: "Used",
    total: "Total",
    optimize: "OPTIMIZE",
    optimizing: "Purging memory lists...",
    freed: "Freed",
    error: "Error cleaning",
    autoCleanIn: "Auto-clean in:",
    seenAngel: "seen this Angel?",
    closeToTray: "Minimize to Tray on Close",
    glassTransparency: "Glass Transparency",
    transparent: "Transparent",
    medium: "Medium",
    solid: "Solid"
  },
  es: {
    options: "Ajustes",
    uiTheme: "Tema de Interfaz",
    language: "Idioma",
    startup: "Iniciar con Windows",
    autoCleanRam: "Auto-limpiar si RAM >",
    autoCleanTimer: "Auto-limpiar cada",
    used: "Usado",
    total: "Total",
    optimize: "OPTIMIZAR",
    optimizing: "Purgando memoria...",
    freed: "Liberado",
    error: "Error al limpiar",
    autoCleanIn: "Auto-limpieza en:",
    seenAngel: "¿has visto a este Ángel?",
    closeToTray: "Minimizar a la Bandeja al Cerrar",
    glassTransparency: "Transparencia Glass",
    transparent: "Transparente",
    medium: "Medio",
    solid: "Solido"
  }
};

const glassOpacityModes: GlassOpacityMode[] = ['transparent', 'medium', 'solid'];

export function useRammyLogic(): RammyLogic {
  const [ramPct, setRamPct] = useState(0);
  const [ramUsed, setRamUsed] = useState("0 GB");
  const [ramTotal, setRamTotal] = useState("0 GB");
  const [isOptimizing, setIsOptimizing] = useState(false);
  const isOptimizingRef = React.useRef(false);
  const [resultText, setResultText] = useState("");

  const [startupEnabled, setStartupEnabled] = useState(false);
  const [timerEnabled, setTimerEnabled] = useState(false);
  const [timerInterval, setTimerInterval] = useState(30);
  
  const [thresholdEnabled, setThresholdEnabled] = useState(false);
  const [ramThreshold, setRamThreshold] = useState(80);

  const [timeLeft, setTimeLeft] = useState(0);
  const [isOverThreshold, setIsOverThreshold] = useState(false);

  const [language, setLanguage] = useState('en');
  const [closeToTray, setCloseToTray] = useState(true);
  const [glassOpacityMode, setGlassOpacityMode] = useState<GlassOpacityMode>('medium');

  const t = (key: string) => {
    return translations[language]?.[key] || key;
  };

  useEffect(() => {
    invoke('is_startup_enabled').then((res: any) => setStartupEnabled(res)).catch(() => {});

    const savedTimerEnabled = localStorage.getItem('timerEnabled') === 'true';
    const savedTimerInterval = parseInt(localStorage.getItem('timerInterval') || '30');
    const savedThresholdEnabled = localStorage.getItem('thresholdEnabled') === 'true';
    const savedThreshold = parseInt(localStorage.getItem('ramThreshold') || '80');
    const savedLanguage = localStorage.getItem('language') || 'en';
    const savedCloseToTray = localStorage.getItem('closeToTray') !== 'false';
    const savedGlassOpacity = localStorage.getItem('glassOpacityMode') as GlassOpacityMode | null;

    setTimerEnabled(savedTimerEnabled);
    setTimerInterval(savedTimerInterval);
    setThresholdEnabled(savedThresholdEnabled);
    setRamThreshold(savedThreshold);
    setTimeLeft(savedTimerInterval * 60);
    setLanguage(savedLanguage);
    setCloseToTray(savedCloseToTray);
    setGlassOpacityMode(savedGlassOpacity && glassOpacityModes.includes(savedGlassOpacity) ? savedGlassOpacity : 'medium');

    const setupNotifications = async () => {
      try {
        const granted = await isPermissionGranted();
        if (!granted) {
          await requestPermission();
        }
      } catch (e) {
        console.error(e);
      }
    };
    setupNotifications();

    const updateStats = async () => {
      try {
        const currentThresholdEnabled = localStorage.getItem('thresholdEnabled') === 'true';
        const currentThreshold = parseInt(localStorage.getItem('ramThreshold') || '80');
        const currentTimerEnabled = localStorage.getItem('timerEnabled') === 'true';
        const currentTimerInterval = parseInt(localStorage.getItem('timerInterval') || '30');

        const stats: any = await invoke('get_ram_stats');
        setRamPct(stats.percent);
        setRamUsed(`${(stats.used / 1024 / 1024 / 1024).toFixed(1)} GB`);
        setRamTotal(`${(stats.total / 1024 / 1024 / 1024).toFixed(1)} GB`);

        if (currentThresholdEnabled) {
          if (stats.percent >= currentThreshold) {
            setIsOverThreshold((prevOver) => {
              if (!prevOver && !isOptimizingRef.current) {
                handleOptimize(true);
                return true;
              }
              return prevOver;
            });
          } else {
            setIsOverThreshold(false);
          }
        }

        if (currentTimerEnabled) {
          setTimeLeft((prev) => {
            if (prev <= 0) {
              if (!isOptimizingRef.current) handleOptimize(true);
              return currentTimerInterval * 60;
            }
            return prev - 1;
          });
        }
      } catch (e) {
        console.error(e);
      }
    };
    
    updateStats();
    const interval = setInterval(updateStats, 1000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    localStorage.setItem('timerEnabled', timerEnabled.toString());
    localStorage.setItem('timerInterval', timerInterval.toString());
    localStorage.setItem('thresholdEnabled', thresholdEnabled.toString());
    localStorage.setItem('ramThreshold', ramThreshold.toString());
    localStorage.setItem('language', language);
    localStorage.setItem('closeToTray', closeToTray.toString());
    localStorage.setItem('glassOpacityMode', glassOpacityMode);
    if (timerEnabled) setTimeLeft(timerInterval * 60);
  }, [timerEnabled, timerInterval, thresholdEnabled, ramThreshold, language, closeToTray, glassOpacityMode]);

  useEffect(() => {
    invoke('set_tray_language', { language }).catch(console.error);
  }, [language]);

  const notifyAutoClean = async (freedBytes: number) => {
    try {
      const granted = await isPermissionGranted();
      if (!granted) return;

      const freedMb = Math.round(freedBytes / 1024 / 1024);
      const body = language === 'es'
        ? `RAMMY liber\u00f3 ${freedMb} MB`
        : `RAMMY freed ${freedMb} MB`;

      sendNotification({ title: 'RAMMY', body });
    } catch (e) {
      console.error(e);
    }
  };

  const toggleStartup = async () => {
    try {
      const newState = !startupEnabled;
      await invoke('set_startup', { enable: newState });
      setStartupEnabled(newState);
    } catch (e) {
      console.error(e);
    }
  };

  const handleOptimize = async (auto = false) => {
    if (isOptimizingRef.current) return;
    setIsOptimizing(true);
    isOptimizingRef.current = true;
    setResultText(auto ? translations[language]?.optimizing || "Purging memory lists..." : translations[language]?.optimizing || "Purging memory lists...");
    try {
      const res: any = await invoke('optimize_ram');
      const gb = (res.freed_bytes / 1024 / 1024 / 1024).toFixed(2);
      setResultText(`${translations[language]?.freed || "Freed"} ${gb} GB`);
      if (auto) {
        await notifyAutoClean(res.freed_bytes);
      }
      setTimeout(() => {
        setResultText("");
        setIsOptimizing(false);
        isOptimizingRef.current = false;
      }, 3000);
    } catch (e) {
      console.error(e);
      setResultText(translations[language]?.error || "Error cleaning");
      setTimeout(() => {
        setIsOptimizing(false);
        isOptimizingRef.current = false;
        setResultText("");
      }, 2000);
    }
  };

  const handleClose = async () => {
    try {
      if (closeToTray) {
        await getCurrentWindow().hide();
      } else {
        await getCurrentWindow().close();
      }
    } catch (error) {
      console.error("Failed to close window:", error);
    }
  };
  
  const handleMinimize = async () => {
    try {
      await getCurrentWindow().hide();
    } catch (error) {
      console.error("Failed to minimize window:", error);
    }
  };

  return {
    ramPct, ramUsed, ramTotal, isOptimizing, resultText,
    startupEnabled, toggleStartup,
    timerEnabled, setTimerEnabled, timerInterval, setTimerInterval, timeLeft,
    thresholdEnabled, setThresholdEnabled, ramThreshold, setRamThreshold,
    handleOptimize, handleClose, handleMinimize,
    language, setLanguage, t, closeToTray, setCloseToTray,
    glassOpacityMode, setGlassOpacityMode
  };
}
