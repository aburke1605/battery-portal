import { StrictMode, useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import { Settings } from 'lucide-react';
import DataChart from './components/charts/DataChart';
import axios from 'axios';
import apiConfig from './apiConfig';

function App() {
  // pick a random esp_id from the battery_info table
  // so we can display a graph beneath
  const [id, setID] = useState("unavailable!");
  useEffect(() => {
    const fetchRandomBattery = async () => {
      try {
        const response = await axios.get(apiConfig.DB_ESP_ID_API);
        if (response.data != null) {
          setID(response.data[0]); // just take the first one
        }
      } catch(err) {
        console.error(err);
      }
    };
    fetchRandomBattery();
  }, []);


  return (
    <div className="min-h-screen flex flex-col bg-white">
      {/* Simple Hero */}
      <div className="flex flex-col items-center justify-center flex-1 text-center px-4 bg-gradient-to-b from-gray-50 to-white">
        <h1 className="text-4xl md:text-5xl font-bold text-blue-600 mb-6">
          Battery Management System Portal
        </h1>
        <p className="text-lg text-gray-600 max-w-xl mb-8">
          Remote monitoring, configuration, and analytics for your energy storage systems.
        </p>
        <a
          href="/admin"
          className="inline-flex items-center bg-blue-600 hover:bg-blue-700 text-white px-6 py-3 rounded-lg shadow-lg transition-colors duration-200 mb-18"
        >
          <Settings className="h-5 w-5 mr-2 animate-pulse" />
          <span className="font-medium">BMS Portal</span>
        </a>

        <div className="bg-blue-600/5 rounded-lg p-6 mb-4">
          <h1 className="text-xl md:text-2xl font-bold text-gray-900 mb-4">
            recent telemetry
          </h1>
          <div className="mx-auto grid grid-cols-1 gap-4 sm:grid-cols-2 max-w-4xl mb-4">
            <div className="w-full aspect-[5/3]">
              <DataChart esp_id={id} column="Q" />
            </div>
            <div className="w-full aspect-[5/3]">
              <DataChart esp_id={id} column="iT" />
            </div>
            <div className="w-full aspect-[5/3]">
              <DataChart esp_id={id} column="V" />
            </div>
            <div className="w-full aspect-[5/3]">
              <DataChart esp_id={id} column="I" />
            </div>
          </div>
          <div className="mx-auto max-w-4xl text-center">
            <p className="text-lg text-gray-600">{id}</p>
          </div>
        </div>
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
            <span>Contact: <a href="mailto:aodhan.burke@liverpool.ac.uk" className="hover:text-blue-400">aodhan.burke@liverpool.ac.uk</a>, <a href="mailto:yunfeng.zhao@liverpool.ac.uk" className="hover:text-blue-400">yunfeng.zhao@liverpool.ac.uk</a></span>
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