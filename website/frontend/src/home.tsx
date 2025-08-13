import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import { Settings } from 'lucide-react';

function App() {
  return (
    <div className="min-h-screen flex flex-col bg-white">
      {/* Simple Hero */}
      <div className="flex flex-col items-center justify-center flex-1 text-center px-4 bg-gradient-to-b from-gray-50 to-white">
        <h1 className="text-4xl md:text-5xl font-bold text-gray-900 mb-6">
          Battery Management System Portal
        </h1>
        <p className="text-lg text-gray-600 max-w-xl mb-8">
          Remote monitoring, configuration, and analytics for your energy storage systems.
        </p>
        <a
          href="/admin"
          className="inline-flex items-center bg-blue-600 hover:bg-blue-700 text-white px-6 py-3 rounded-lg shadow-lg transition-colors duration-200"
        >
          <Settings className="h-5 w-5 mr-2 animate-pulse" />
          <span className="font-medium">BMS Portal</span>
        </a>
      </div>

      {/* Footer */}
      <footer className="bg-gray-900 text-white py-6">
        <div className="container mx-auto px-4 flex flex-col md:flex-row justify-between items-center">
          <span className="text-lg font-bold">
            <a href="https://www.aceongroup.com/news/innovate-project-highess-wins-1-4m-for-2nd-life-mini-grid-in-sub-sahara-africa/">
              HIGHESS
            </a>
            {" and "}
            <a href="https://www.aceongroup.com/news/project-mettle/">
              METTLE
            </a>
            {" Projects"}
          </span>
          <div className="flex flex-col md:flex-row items-center text-gray-400 mt-2 md:mt-0 space-y-2 md:space-y-0 md:space-x-6">
            <span>Contact: <a href="mailto:aodhan.burke@liverpool.ac.uk" className="hover:text-blue-400">aodhan.burke@liverpool.ac.uk</a></span>
            <span>&copy; {new Date().getFullYear()} University of Liverpool</span>
          </div>
        </div>
      </footer>
    </div>
  );
}

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>
);