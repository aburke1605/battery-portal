import { useEffect, useState } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import { useModal } from "../../hooks/useModal";
import { User } from '../../types/index';
import { Modal } from "../../components/ui/modal";
import Button from "../../components/ui/button/Button";
import Input from "../../components/form/input/InputField";
import Label from "../../components/form/Label";
import { 
  UserPlus, 
  Edit3, 
  Trash2, 
  ChevronLeft,
  ChevronRight,
  AlertTriangle,
  AlertCircle
} from 'lucide-react';
import axios from 'axios';
import apiConfig from '../../apiConfig';

const UserList = () => {
  const [users, setUsers] = useState<User[]>([]);
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [pages, setPages] = useState<number>(1);
  const [loading, setLoading] = useState<boolean>(false);
  const [isEditMode, setIsEditMode] = useState<boolean>(false);
  const [editingUser, setEditingUser] = useState<User | null>(null);
  const [formData, setFormData] = useState({
    first_name: '',
    last_name: '',
    email: '',
    password: '',
  });
  const [errors, setErrors] = useState<{[key: string]: string}>({});
  const [showDeleteModal, setShowDeleteModal] = useState<boolean>(false);
  const [userToDelete, setUserToDelete] = useState<User | null>(null);
  const [deleteLoading, setDeleteLoading] = useState<boolean>(false);
  const [deleteError, setDeleteError] = useState<string>('');

  const fetchUsers = async (page = 1) => {
    setLoading(true);
    try {
      const response = await axios.get(`${apiConfig.USER_API}/list?page=${page}&per_page=10`);
      setUsers(response.data.users);
      setCurrentPage(response.data.current_page);
      setPages(response.data.pages);
    } catch (err) {
      console.error('Failed to fetch users:', err);
    }
    setLoading(false);
  };

  useEffect(() => {
    fetchUsers(currentPage);
  }, [currentPage]);

  const handlePrev = () => {
    if (currentPage > 1) setCurrentPage(prev => prev - 1);
  };

  const handleNext = () => {
    if (currentPage < pages) setCurrentPage(prev => prev + 1);
  };

  // const handleAddUser = () => {
  //   // Placeholder: Navigate to Add User form or open modal
  //   alert('Navigate to Add User page or open modal');
  // };

  // const handleEditUser = (userId: number) => {
  //   // Placeholder: Navigate to edit page or open modal
  //   alert(`Edit user with ID: ${userId}`);
  // };

  const { isOpen, openModal, closeModal } = useModal();

  const validateForm = () => {
    const newErrors: {[key: string]: string} = {};

    // Required field validation
    if (!formData.first_name.trim()) {
      newErrors.first_name = 'First name is required';
    }

    if (!formData.last_name.trim()) {
      newErrors.last_name = 'Last name is required';
    }

    if (!formData.email.trim()) {
      newErrors.email = 'Email is required';
    } else if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(formData.email)) {
      newErrors.email = 'Please enter a valid email address';
    }

    // Password validation - required only for new users
    if (!isEditMode && !formData.password.trim()) {
      newErrors.password = 'Password is required';
    } else if (formData.password && formData.password.length < 6) {
      newErrors.password = 'Password must be at least 6 characters long';
    }

    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const handleAddUser = () => {
    setIsEditMode(false);
    setEditingUser(null);
    setFormData({
      first_name: '',
      last_name: '',
      email: '',
      password: '',
    });
    setErrors({});
    openModal();
  };

  const handleEditUser = (user: User) => {
    setIsEditMode(true);
    setEditingUser(user);
    setFormData({
      first_name: user.first_name || '',
      last_name: user.last_name || '',
      email: user.email || '',
      password: '', // Don't pre-fill password for security
    });
    setErrors({});
    openModal();
  };

  const handleCloseModal = () => {
    setIsEditMode(false);
    setEditingUser(null);
    setFormData({
      first_name: '',
      last_name: '',
      email: '',
      password: '',
    });
    setErrors({});
    closeModal();
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    setFormData(prev => ({ ...prev, [name]: value }));
    
    // Clear error for this field when user starts typing
    if (errors[name]) {
      setErrors(prev => ({ ...prev, [name]: '' }));
    }
  };

  const handleSubmit = async () => {
    // Validate form before submitting
    if (!validateForm()) {
      return;
    }

    const payload = {
      ...formData
    };

    let unknown_error = true; // assume unknown error, change if error known or no error
    try {
      let res;
      if (isEditMode && editingUser) {
        // Edit existing user
        res = await axios.put(`${apiConfig.USER_API}/${editingUser.id}`, payload);
      } else {
        // Add new user
        res = await axios.post(`${apiConfig.USER_API}/add`, payload);
      }

      let message = res.data.success;
      if (message) {
        handleCloseModal();
        fetchUsers(currentPage);
        unknown_error = false;
      }
    } catch (err) {
      if (axios.isAxiosError(err)) {
        let res = err.response;
        if (res) {
          let message = res.data.error;
          if (message) {
            if (message.includes('already exists')) {
              setErrors({email: message})
            } else {
              setErrors({general: message})
            }
            unknown_error = false;
          }
        }
      }
    }
    if (unknown_error) setErrors({general: "Unknown error occurred"});
  };

  const handleDeleteClick = (user: User) => {
    setUserToDelete(user);
    setDeleteError('');
    setShowDeleteModal(true);
  };

  const handleDeleteCancel = () => {
    setUserToDelete(null);
    setDeleteError('');
    setShowDeleteModal(false);
  };

  const handleDeleteConfirm = async () => {
    if (!userToDelete) return;

    setDeleteLoading(true);
    setDeleteError('');
    
    let unknown_error = true; // assume unknown error, change if error known or no error
    try {
      const res = await axios.delete(`${apiConfig.USER_API}/${userToDelete.id}`);

      let message = res.data.success;
      if (message) {
        handleDeleteCancel();
        fetchUsers(currentPage);
        unknown_error = false;
      }
    } catch (err) {
      if (axios.isAxiosError(err)) {
        let res = err.response;
        if (res) {
          let message = res.data.error;
          if (message) {
            setDeleteError(message);
            unknown_error = false;
          }
        }
      }
    } finally {
      setDeleteLoading(false);
    }
    if (unknown_error) setDeleteError("Unknown error occurred");
  };

  return (
    <>
    <div className="space-y-6">
      <PageBreadcrumb pageTitle="Users" />
      
      {/* Header Section */}
      <div className="bg-white dark:bg-gray-900 rounded-xl shadow-sm border border-gray-200 dark:border-gray-700">
        <div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4">
            <div>
              <h1 className="text-2xl font-semibold text-gray-900 dark:text-white">Users</h1>
              <p className="text-sm text-gray-600 dark:text-gray-400 mt-1">
                Manage user accounts and permissions
              </p>
            </div>
            <button
              onClick={handleAddUser}
              className="inline-flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg font-medium transition-colors shadow-sm"
            >
              <UserPlus className="w-5 h-5" />
              Add User
            </button>
          </div>
        </div>

        {/* Table Section */}
        <div className="overflow-hidden">
          {loading ? (
            <div className="flex items-center justify-center py-12">
              <div className="flex items-center gap-3 text-gray-500 dark:text-gray-400">
                <div className="animate-spin rounded-full h-6 w-6 border-b-2 border-blue-600"></div>
                <span>Loading users...</span>
              </div>
            </div>
          ) : users.length === 0 ? (
            <div className="text-center py-12">
              <div className="mx-auto w-24 h-24 bg-gray-100 dark:bg-gray-800 rounded-full flex items-center justify-center mb-4">
                <UserPlus className="w-12 h-12 text-gray-400" />
              </div>
              <h3 className="text-lg font-medium text-gray-900 dark:text-white mb-2">No users found</h3>
              <p className="text-gray-500 dark:text-gray-400 mb-6">Get started by adding your first user.</p>
              <button
                onClick={handleAddUser}
                className="inline-flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg font-medium transition-colors"
              >
                <UserPlus className="w-5 h-5" />
                Add User
              </button>
            </div>
          ) : (
            <>
              <div className="overflow-x-auto">
                <table className="w-full">
                  <thead className="bg-gray-50 dark:bg-gray-800">
                    <tr>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 dark:text-gray-400 uppercase tracking-wider">
                        User
                      </th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 dark:text-gray-400 uppercase tracking-wider">
                        Email
                      </th>
                      <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 dark:text-gray-400 uppercase tracking-wider">
                        Actions
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white dark:bg-gray-900 divide-y divide-gray-200 dark:divide-gray-700">
                    {users.map((user) => (
                      <tr key={user.id} className="hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors">
                        <td className="px-6 py-4 whitespace-nowrap">
                          <div className="flex items-center">
                            <div className="flex-shrink-0 h-10 w-10">
                              <div className="h-10 w-10 rounded-full bg-gradient-to-r from-blue-500 to-purple-600 flex items-center justify-center">
                                <span className="text-sm font-medium text-white">
                                  {user.first_name?.[0]?.toUpperCase()}{user.last_name?.[0]?.toUpperCase()}
                                </span>
                              </div>
                            </div>
                            <div className="ml-4">
                              <div className="text-sm font-medium text-gray-900 dark:text-white">
                                {user.first_name} {user.last_name}
                              </div>
                              <div className="text-sm text-gray-500 dark:text-gray-400">
                                ID: {user.id}
                              </div>
                            </div>
                          </div>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap">
                          <div className="text-sm text-gray-900 dark:text-white">{user.email}</div>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-right text-sm font-medium">
                          <div className="flex items-center justify-end gap-2">
                            <button
                              onClick={() => handleEditUser(user)}
                              className="inline-flex items-center p-2 text-gray-400 hover:text-blue-600 hover:bg-blue-50 dark:hover:bg-blue-900/20 rounded-lg transition-colors"
                              title="Edit user"
                            >
                              <Edit3 className="w-4 h-4" />
                            </button>
                            <button
                              onClick={() => handleDeleteClick(user)}
                              className="inline-flex items-center p-2 text-gray-400 hover:text-red-600 hover:bg-red-50 dark:hover:bg-red-900/20 rounded-lg transition-colors"
                              title="Delete user"
                            >
                              <Trash2 className="w-4 h-4" />
                            </button>
                          </div>
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>

              {/* Modern Pagination */}
              <div className="px-6 py-4 border-t border-gray-200 dark:border-gray-700">
                <div className="flex items-center justify-between">
                  <div className="text-sm text-gray-700 dark:text-gray-300">
                    Showing <span className="font-medium">{((currentPage - 1) * 10) + 1}</span> to{' '}
                    <span className="font-medium">{Math.min(currentPage * 10, users.length)}</span> of{' '}
                    <span className="font-medium">{users.length}</span> results
                  </div>
                  
                  <nav className="flex items-center gap-1">
                    <button
                      onClick={handlePrev}
                      disabled={currentPage === 1}
                      className="inline-flex items-center gap-2 px-3 py-2 text-sm font-medium text-gray-500 bg-white border border-gray-300 rounded-lg hover:bg-gray-50 hover:text-gray-700 disabled:opacity-50 disabled:cursor-not-allowed dark:bg-gray-800 dark:border-gray-600 dark:text-gray-400 dark:hover:bg-gray-700 dark:hover:text-gray-300"
                    >
                      <ChevronLeft className="w-4 h-4" />
                      Previous
                    </button>
                    
                    <div className="flex items-center gap-1 mx-2">
                      {Array.from({ length: Math.min(pages, 5) }, (_, i) => {
                        const pageNumber = i + 1;
                        const isCurrentPage = pageNumber === currentPage;
                        return (
                          <button
                            key={pageNumber}
                            onClick={() => setCurrentPage(pageNumber)}
                            className={`px-3 py-2 text-sm font-medium rounded-lg transition-colors ${
                              isCurrentPage
                                ? 'bg-blue-600 text-white'
                                : 'text-gray-700 hover:bg-gray-100 dark:text-gray-300 dark:hover:bg-gray-700'
                            }`}
                          >
                            {pageNumber}
                          </button>
                        );
                      })}
                      {pages > 5 && (
                        <>
                          <span className="px-2 text-gray-500">...</span>
                          <button
                            onClick={() => setCurrentPage(pages)}
                            className={`px-3 py-2 text-sm font-medium rounded-lg transition-colors ${
                              pages === currentPage
                                ? 'bg-blue-600 text-white'
                                : 'text-gray-700 hover:bg-gray-100 dark:text-gray-300 dark:hover:bg-gray-700'
                            }`}
                          >
                            {pages}
                          </button>
                        </>
                      )}
                    </div>
                    
                    <button
                      onClick={handleNext}
                      disabled={currentPage === pages}
                      className="inline-flex items-center gap-2 px-3 py-2 text-sm font-medium text-gray-500 bg-white border border-gray-300 rounded-lg hover:bg-gray-50 hover:text-gray-700 disabled:opacity-50 disabled:cursor-not-allowed dark:bg-gray-800 dark:border-gray-600 dark:text-gray-400 dark:hover:bg-gray-700 dark:hover:text-gray-300"
                    >
                      Next
                      <ChevronRight className="w-4 h-4" />
                    </button>
                  </nav>
                </div>
              </div>
            </>
          )}
        </div>
      </div>
    </div>
    
         <Modal 
       isOpen={isOpen} 
       onClose={handleCloseModal} 
       className="max-w-2xl m-4"
       closeOnBackdropClick={false}
       backdropOpacity="bg-gray-900/75"
     >
        <div className="relative w-full max-w-2xl bg-white dark:bg-gray-900 rounded-2xl shadow-xl overflow-hidden">
          {/* Modal Header */}
          <div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-3">
                <div className="p-2 bg-blue-100 dark:bg-blue-900/20 rounded-lg">
                  {isEditMode ? (
                    <Edit3 className="w-6 h-6 text-blue-600 dark:text-blue-400" />
                  ) : (
                    <UserPlus className="w-6 h-6 text-blue-600 dark:text-blue-400" />
                  )}
                </div>
                <div>
                  <h3 className="text-lg font-semibold text-gray-900 dark:text-white">
                    {isEditMode ? 'Edit User' : 'Add New User'}
                  </h3>
                  <p className="text-sm text-gray-500 dark:text-gray-400">
                    {isEditMode 
                      ? 'Update user account information' 
                      : 'Create a new user account with the required information'
                    }
                  </p>
                </div>
              </div>
              <button
                onClick={handleCloseModal}
                className="p-2 text-gray-400 hover:text-gray-600 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            </div>
          </div>

          {/* Modal Body */}
          <form className="flex flex-col" onSubmit={(e) => e.preventDefault()}>
            <div className="px-6 py-6 space-y-6 max-h-96 overflow-y-auto">
              {/* Validation Error Summary */}
              {Object.keys(errors).length > 0 && (
                <div className="p-4 bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-lg">
                  <div className="flex items-start">
                    <AlertTriangle className="w-5 h-5 text-red-600 dark:text-red-400 mt-0.5 mr-3 flex-shrink-0" />
                    <div>
                      <h4 className="text-sm font-medium text-red-800 dark:text-red-200">
                        {errors.general ? 'Error' : 'Please fix the following errors:'}
                      </h4>
                      {errors.general ? (
                        <p className="mt-2 text-sm text-red-700 dark:text-red-300">
                          {errors.general}
                        </p>
                      ) : (
                        <ul className="mt-2 text-sm text-red-700 dark:text-red-300 list-disc list-inside space-y-1">
                          {Object.entries(errors)
                            .filter(([key]) => key !== 'general')
                            .map(([, error], index) => (
                              <li key={index}>{error}</li>
                            ))}
                        </ul>
                      )}
                    </div>
                  </div>
                </div>
              )}
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
                <div>
                  <Label>First Name *</Label>
                  <Input
                    name="first_name"
                    type="text"
                    value={formData.first_name}
                    onChange={handleInputChange}
                    placeholder="Enter first name"
                    className={`mt-1 ${errors.first_name ? 'border-red-500 focus:ring-red-500' : ''}`}
                  />
                  {errors.first_name && (
                    <p className="mt-1 text-sm text-red-600 dark:text-red-400">
                      {errors.first_name}
                    </p>
                  )}
                </div>

                <div>
                  <Label>Last Name *</Label>
                  <Input 
                    name="last_name"
                    type="text" 
                    value={formData.last_name}
                    onChange={handleInputChange}
                    placeholder="Enter last name"
                    className={`mt-1 ${errors.last_name ? 'border-red-500 focus:ring-red-500' : ''}`}
                  />
                  {errors.last_name && (
                    <p className="mt-1 text-sm text-red-600 dark:text-red-400">
                      {errors.last_name}
                    </p>
                  )}
                </div>
              </div>

              <div>
                <Label>Email Address *</Label>
                <Input
                  name="email"
                  type="email"
                  value={formData.email}
                  onChange={handleInputChange}
                  placeholder="Enter email address"
                  className={`mt-1 ${errors.email ? 'border-red-500 focus:ring-red-500' : ''}`}
                />
                {errors.email && (
                  <p className="mt-1 text-sm text-red-600 dark:text-red-400">
                    {errors.email}
                  </p>
                )}
              </div>

              <div>
                <Label>Password {!isEditMode && '*'}</Label>
                <Input 
                  name="password"
                  type="password"
                  value={formData.password}
                  onChange={handleInputChange}
                  placeholder={isEditMode ? "Leave empty to keep current password" : "Enter password"}
                  className={`mt-1 ${errors.password ? 'border-red-500 focus:ring-red-500' : ''}`}
                />
                {errors.password && (
                  <p className="mt-1 text-sm text-red-600 dark:text-red-400">
                    {errors.password}
                  </p>
                )}
                {isEditMode && !errors.password && (
                  <p className="mt-1 text-xs text-gray-500 dark:text-gray-400">
                    Leave empty to keep the current password
                  </p>
                )}
              </div>
            </div>

            {/* Modal Footer */}
            <div className="px-6 py-4 border-t border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50">
              <div className="flex items-center justify-end gap-3">
                <Button size="sm" variant="outline" onClick={handleCloseModal}>
                  Cancel
                </Button>
                <Button 
                  size="sm" 
                  onClick={handleSubmit}
                  className="bg-blue-600 hover:bg-blue-700 text-white"
                >
                  {isEditMode ? 'Update User' : 'Create User'}
                </Button>
              </div>
            </div>
          </form>
        </div>
      </Modal>

             {/* Delete Confirmation Modal */}
       <Modal 
         isOpen={showDeleteModal} 
         onClose={handleDeleteCancel} 
         className="max-w-md m-4"
         closeOnBackdropClick={false}
         backdropOpacity="bg-gray-900/80"
       >
        <div className="relative w-full max-w-md bg-white dark:bg-gray-900 rounded-2xl shadow-xl overflow-hidden">
          {/* Modal Header */}
          <div className="px-6 py-4 border-b border-gray-200 dark:border-gray-700">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-3">
                <div className="p-2 bg-red-100 dark:bg-red-900/20 rounded-lg">
                  <AlertCircle className="w-6 h-6 text-red-600 dark:text-red-400" />
                </div>
                <div>
                  <h3 className="text-lg font-semibold text-gray-900 dark:text-white">
                    Delete User
                  </h3>
                  <p className="text-sm text-gray-500 dark:text-gray-400">
                    This action cannot be undone
                  </p>
                </div>
              </div>
              <button
                onClick={handleDeleteCancel}
                className="p-2 text-gray-400 hover:text-gray-600 hover:bg-gray-100 dark:hover:bg-gray-700 rounded-lg transition-colors"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            </div>
          </div>

          {/* Modal Body */}
          <div className="px-6 py-6">
            {/* Error Message */}
            {deleteError && (
              <div className="mb-4 p-4 bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-lg">
                <div className="flex items-start">
                  <AlertTriangle className="w-5 h-5 text-red-600 dark:text-red-400 mt-0.5 mr-3 flex-shrink-0" />
                  <div>
                    <h4 className="text-sm font-medium text-red-800 dark:text-red-200">
                      Delete Failed
                    </h4>
                    <p className="mt-1 text-sm text-red-700 dark:text-red-300">
                      {deleteError}
                    </p>
                  </div>
                </div>
              </div>
            )}
            
            <div className="text-center">
              <div className="mx-auto w-16 h-16 bg-red-100 dark:bg-red-900/20 rounded-full flex items-center justify-center mb-4">
                <Trash2 className="w-8 h-8 text-red-600 dark:text-red-400" />
              </div>
              <h4 className="text-lg font-medium text-gray-900 dark:text-white mb-2">
                Are you sure you want to delete this user?
              </h4>
              {userToDelete && (
                <div className="mb-4 p-3 bg-gray-50 dark:bg-gray-800 rounded-lg">
                  <p className="text-sm font-medium text-gray-900 dark:text-white">
                    {userToDelete.first_name} {userToDelete.last_name}
                  </p>
                  <p className="text-sm text-gray-500 dark:text-gray-400">
                    {userToDelete.email}
                  </p>
                </div>
              )}
            </div>
          </div>

          {/* Modal Footer */}
          <div className="px-6 py-4 border-t border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50">
            <div className="flex items-center justify-end gap-3">
              <Button 
                size="sm" 
                variant="outline" 
                onClick={handleDeleteCancel}
                disabled={deleteLoading}
              >
                Cancel
              </Button>
              <Button 
                size="sm" 
                onClick={handleDeleteConfirm}
                disabled={deleteLoading}
                className="bg-red-600 hover:bg-red-700 text-white"
              >
                {deleteLoading ? 'Deleting...' : 'Delete User'}
              </Button>
            </div>
          </div>
        </div>
      </Modal>
    </>
  );
};

export default UserList;
