import React from 'react';
import { ChevronRight } from 'lucide-react';

interface BreadcrumbProps {
  pageName: string;
}

const Breadcrumb: React.FC<BreadcrumbProps> = ({ pageName }) => {
  return (
    <div className="flex items-center text-sm text-gray-500 mb-4">
      <a href="#" className="hover:text-blue-600">Home</a>
      <ChevronRight size={16} className="mx-1" />
      <span className="text-gray-700">{pageName}</span>
    </div>
  );
};

export default Breadcrumb;