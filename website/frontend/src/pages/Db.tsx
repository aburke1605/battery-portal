import React, { useState } from 'react';
import PageMeta from "../components/common/PageMeta";
import PageBreadcrumb from "../components/common/PageBreadCrumb";
import axios from 'axios';
import apiConfig from '../apiConfig';

interface QueryResult {
  [key: string]: string | number | null;
}

const SqlQueryPage: React.FC = () => {
  const [query, setQuery] = useState<string>('SELECT * FROM your_table LIMIT 10;');
  const [results, setResults] = useState<QueryResult[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const handleRunQuery = async () => {
    setLoading(true);
    setError(null);
    setResults([]);

    try {
      const response = await axios.post(apiConfig.DB_EXECUTE_SQL_API, { query });
      if (response.data.rows) setResults(response.data.rows);
    } catch (err: any) {
      setError(err.response?.data?.error || err.message);
    } finally {
      setLoading(false);
    }
  };

  const headers = results.length > 0 ? Object.keys(results[0]) : [];

  return (
    <>
    <PageMeta
        title="Database Query Runner"
        description="Database Query Runner"
      />
      <PageBreadcrumb pageTitle="Database Query" />
    <div className="p-6">

      <textarea
        className="w-full p-4 text-sm border rounded-lg resize-none mb-4 h-40"
        value={query}
        onChange={(e) => setQuery(e.target.value)}
        placeholder="Enter your SQL SELECT statement here..."
      />

      <button
        className="bg-blue-600 hover:bg-blue-700 text-white font-semibold py-2 px-6 rounded mb-6"
        onClick={handleRunQuery}
        disabled={loading}
      >
        {loading ? 'Running...' : 'Run Query'}
      </button>

      {error && <p className="text-red-500 mb-4">Error: {error}</p>}

      {results.length > 0 && (
        <div className="overflow-x-auto">
          <table className="min-w-full text-sm text-left border border-gray-300">
            <thead className="bg-gray-100">
              <tr>
                {headers.map((col) => (
                  <th key={col} className="px-4 py-2 border-b border-gray-300 font-medium">
                    {col}
                  </th>
                ))}
              </tr>
            </thead>
            <tbody>
              {results.map((row, i) => (
                <tr key={i} className="even:bg-gray-50">
                  {headers.map((col) => (
                    <td key={col} className="px-4 py-2 border-b border-gray-200">
                      {row[col] !== null ? row[col] : <span className="text-gray-400">NULL</span>}
                    </td>
                  ))}
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
    </>
  );
};

export default SqlQueryPage;
