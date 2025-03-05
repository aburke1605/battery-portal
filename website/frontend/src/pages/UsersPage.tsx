import React from 'react';
import { UserData } from '../types';
import Breadcrumb from '../components/Breadcrumb';
import { UserPlus, Settings, Search, ChevronDown } from 'lucide-react';
import { formatDateTime } from '../utils/helpers';

interface UsersPageProps {
  users: UserData[];
}

const UsersPage: React.FC<UsersPageProps> = ({ users }) => {
  return (
    <>
      {/* Breadcrumbs */}
      <Breadcrumb pageName="Users" />

      {/* Users Header */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 p-6 mb-6">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between">
          <div>
            <h2 className="text-xl font-semibold text-gray-800 mb-1">System Users</h2>
            <p className="text-sm text-gray-600">
              Manage user accounts and permissions
            </p>
          </div>
          <div className="mt-4 md:mt-0">
            <button className="px-4 py-2 bg-blue-600 text-white rounded-md text-sm font-medium hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500 flex items-center">
              <UserPlus size={16} className="mr-2" />
              Add New User
            </button>
          </div>
        </div>
      </div>

      {/* Users List */}
      <div className="bg-white rounded-lg shadow-sm border border-gray-200 overflow-hidden">
        <div className="px-4 py-5 sm:px-6 border-b border-gray-200 bg-gray-50 flex justify-between items-center">
          <h3 className="text-lg font-medium leading-6 text-gray-900">All Users</h3>
          <div className="flex items-center space-x-2">
            <div className="relative">
              <select 
                className="appearance-none bg-white border rounded-md px-4 py-2 pr-8 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 text-sm"
              >
                <option value="all">All Roles</option>
                <option value="admin">Administrators</option>
                <option value="tech">Technicians</option>
                <option value="operator">Operators</option>
              </select>
              <ChevronDown size={16} className="absolute right-2 top-2.5 text-gray-500 pointer-events-none" />
            </div>
            <div className="relative">
              <input
                type="text"
                placeholder="Search users..."
                className="appearance-none bg-white border rounded-md px-4 py-2 pr-8 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 text-sm"
              />
              <Search size={16} className="absolute right-2 top-2.5 text-gray-500 pointer-events-none" />
            </div>
          </div>
        </div>
        <ul className="divide-y divide-gray-200">
          {users.map(user => (
            <li key={user.id} className="p-4 hover:bg-gray-50">
              <div className="flex items-center justify-between">
                <div className="flex items-center">
                  <div className="h-10 w-10 rounded-full bg-blue-100 flex items-center justify-center text-blue-700 font-medium">
                    {user.avatar}
                  </div>
                  <div className="ml-4">
                    <h4 className="text-base font-medium text-gray-900">{user.name}</h4>
                    <div className="flex items-center text-sm text-gray-500">
                      <span>{user.email}</span>
                      <span className="mx-2">•</span>
                      <span>{user.role}</span>
                    </div>
                  </div>
                </div>
                <div className="flex items-center">
                  <span className="text-sm text-gray-500 mr-4">
                    Last active: {formatDateTime(user.lastActive)}
                  </span>
                  <div className="flex space-x-2">
                    <button className="p-2 text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-full">
                      <Settings size={18} />
                    </button>
                  </div>
                </div>
              </div>
            </li>
          ))}
        </ul>
      </div>
    </>
  );
};

export default UsersPage;