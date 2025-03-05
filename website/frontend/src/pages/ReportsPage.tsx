import React from 'react';
import Breadcrumb from '../components/Breadcrumb';
import { 
  FileText, 
  BarChart3, 
  PieChart, 
  LineChart, 
  Download, 
  Share2 
} from 'lucide-react';

const ReportsPage: React.FC = () => {
  return (
    <>
      {/* Breadcrumbs */}
      <Breadcrumb pageName="Reports" />

      {/* Reports Header */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between">
          <div>
            <h2 className="text-xl font-semibold text-gray-800 mb-1">Battery System Reports</h2>
            <p className="text-sm text-gray-600">
              Generate and view system performance reports
            </p>
          </div>
          <div className="mt-4 md:mt-0">
            <button className="px-4 py-2 bg-blue-600 text-white rounded-md text-sm font-medium hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 flex items-center">
              <FileText size={16} className="mr-2" />
              Generate New Report
            </button>
          </div>
        </div>
      </div>

      {/* Report Types */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-6">
        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 hover:shadow-md transition-shadow">
          <div className="flex items-center mb-4">
            <div className="h-12 w-12 rounded-lg bg-blue-100 flex items-center justify-center">
              <BarChart3 size={24} className="text-blue-600" />
            </div>
            <h3 className="ml-4 text-lg font-medium text-gray-900">Performance Report</h3>
          </div>
          <p className="text-sm text-gray-600 mb-4">
            Detailed analysis of battery performance metrics including charge cycles, efficiency, and degradation over time.
          </p>
          <button className="w-full px-4 py-2 border border-blue-300 rounded-md text-sm font-medium text-blue-700 bg-blue-50 hover:bg-blue-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500">
            Generate Report
          </button>
        </div>

        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 hover:shadow-md transition-shadow">
          <div className="flex items-center mb-4">
            <div className="h-12 w-12 rounded-lg bg-green-100 flex items-center justify-center">
              <PieChart size={24} className="text-green-600" />
            </div>
            <h3 className="ml-4 text-lg font-medium text-gray-900">Health Status Report</h3>
          </div>
          <p className="text-sm text-gray-600 mb-4">
            Comprehensive overview of battery health, maintenance history, and projected lifespan for all units.
          </p>
          <button className="w-full px-4 py-2 border border-green-300 rounded-md text-sm font-medium text-green-700 bg-green-50 hover:bg-green-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500">
            Generate Report
          </button>
        </div>

        <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 hover:shadow-md transition-shadow">
          <div className="flex items-center mb-4">
            <div className="h-12 w-12 rounded-lg bg-purple-100 flex items-center justify-center">
              <LineChart size={24} className="text-purple-600" />
            </div>
            <h3 className="ml-4 text-lg font-medium text-gray-900">Efficiency Report</h3>
          </div>
          <p className="text-sm text-gray-600 mb-4">
            Analysis of energy efficiency, charging patterns, and optimization recommendations for the battery system.
          </p>
          <button className="w-full px-4 py-2 border border-purple-300 rounded-md text-sm font-medium text-purple-700 bg-purple-50 hover:bg-purple-100 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-purple-500">
            Generate Report
          </button>
        </div>
      </div>

      {/* Recent Reports */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden">
        <div className="px-4 py-5 sm:px-6 border-b border-gray-200 bg-gray-50">
          <h3 className="text-lg font-medium leading-6 text-gray-900">Recent Reports</h3>
        </div>
        <ul className="divide-y divide-gray-200">
          <li className="p-4 hover:bg-gray-50">
            <div className="flex items-center justify-between">
              <div className="flex items-center">
                <div className="h-10 w-10 rounded-full bg-blue-100 flex items-center justify-center">
                  <BarChart3 size={20} className="text-blue-600" />
                </div>
                <div className="ml-4">
                  <h4 className="text-base font-medium text-gray-900">Monthly Performance Summary</h4>
                  <p className="text-sm text-gray-500">Generated on April 1, 2025</p>
                </div>
              </div>
              <div className="flex space-x-2">
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Download size={18} />
                </button>
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Share2 size={18} />
                </button>
              </div>
            </div>
          </li>
          <li className="p-4 hover:bg-gray-50">
            <div className="flex items-center justify-between">
              <div className="flex items-center">
                <div className="h-10 w-10 rounded-full bg-green-100 flex items-center justify-center">
                  <PieChart size={20} className="text-green-600" />
                </div>
                <div className="ml-4">
                  <h4 className="text-base font-medium text-gray-900">Quarterly Health Assessment</h4>
                  <p className="text-sm text-gray-500">Generated on March 15, 2025</p>
                </div>
              </div>
              <div className="flex space-x-2">
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Download size={18} />
                </button>
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Share2 size={18} />
                </button>
              </div>
            </div>
          </li>
          <li className="p-4 hover:bg-gray-50">
            <div className="flex items-center justify-between">
              <div className="flex items-center">
                <div className="h-10 w-10 rounded-full bg-purple-100 flex items-center justify-center">
                  <LineChart size={20} className="text-purple-600" />
                </div>
                <div className="ml-4">
                  <h4 className="text-base font-medium text-gray-900">Annual Efficiency Report</h4>
                  <p className="text-sm text-gray-500">Generated on February 28, 2025</p>
                </div>
              </div>
              <div className="flex space-x-2">
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Download size={18} />
                </button>
                <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                  <Share2 size={18} />
                </button>
              </div>
            </div>
          </li>
        </ul>
      </div>
    </>
  );
};

export default ReportsPage;