import { useEffect, useState } from 'react';
import PageBreadcrumb from "../../components/common/PageBreadCrumb";
import { useModal } from "../../hooks/useModal";
import { User } from '../../types/index';
import { Modal } from "../../components/ui/modal";
import Button from "../../components/ui/button/Button";
import Input from "../../components/form/input/InputField";
import Label from "../../components/form/Label";
import axios from 'axios';

const UserList = () => {
  const [users, setUsers] = useState<User[]>([]);
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [pages, setPages] = useState<number>(1);
  const [loading, setLoading] = useState<boolean>(false);
  const [formData, setFormData] = useState({
    first_name: '',
    last_name: '',
    email: '',
    password: '',
    roles: '',
  });

  const fetchUsers = async (page = 1) => {
    setLoading(true);
    try {
      const response = await axios.get(`/api/users/list?page=${page}&per_page=10`);
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

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = e.target;
    console.log(name);
    setFormData(prev => ({ ...prev, [name]: value }));
  };

  const handleAddUserSubmit = async () => {
    //e.preventDefault();
    const payload = {
      ...formData,
      roles: formData.roles.split(',').map(r => r.trim())
    };

    try {
      const res = await fetch('/api/users/add', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });

      if (res.ok) {
        closeModal();
        setFormData({ first_name: '', last_name: '', email: '', password: '', roles: '' });
        fetchUsers(currentPage);
      } else {
        console.error('Failed to add user');
      }
    } catch (err) {
      console.error('Error adding user:', err);
    }
  };

  return (
    <>  
    <PageBreadcrumb pageTitle="Users" />
    <div className="p-5 border border-gray-200 rounded-2xl dark:border-gray-800 lg:p-6">
    <div className="p-6">
      <div className="flex justify-between items-center mb-4">
        <button
          //onClick={handleAddUser}
          onClick={openModal}
          className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700"
        >
          + Add User
        </button>
        <div /> {/* empty to maintain spacing */}
      </div>

      {loading ? (
        <p className="text-gray-500">Loading...</p>
      ) : (
        <>
          <table className="min-w-full bg-white border border-gray-200 rounded-lg overflow-hidden shadow">
            <thead className="">
              <tr>
                <th className="py-2 px-4 text-left">ID</th>
                <th className="py-2 px-4 text-left">First Name</th>
                <th className="py-2 px-4 text-left">Last Name</th>
                <th className="py-2 px-4 text-left">Email</th>
                <th className="py-2 px-4 text-left">Roles</th>
                <th className="py-2 px-4 text-left">Actions</th>
              </tr>
            </thead>
            <tbody>
              {users.map(user => (
                <tr key={user.id} className="border-t border-gray-200">
                  <td className="py-2 px-4">{user.id}</td>
                  <td className="py-2 px-4">{user.first_name}</td>
                  <td className="py-2 px-4">{user.last_name}</td>
                  <td className="py-2 px-4">{user.email}</td>
                  <td className="py-2 px-4">{user.roles.join(', ')}</td>
                  <td className="py-2 px-4">
                    <button
                      //onClick={() => handleEditUser(user.id)}
                      onClick={openModal}
                      className="text-blue-600 hover:underline"
                    >
                      Edit
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>

          {/* Pagination Controls */}
          <div className="flex justify-between items-center mt-4">
            <button
              onClick={handlePrev}
              disabled={currentPage === 1}
              className="px-4 py-2 bg-gray-200 text-gray-700 rounded disabled:opacity-50"
            >
              Previous
            </button>

            <span className="text-gray-700">Page {currentPage} of {pages}</span>

            <button
              onClick={handleNext}
              disabled={currentPage === pages}
              className="px-4 py-2 bg-gray-200 text-gray-700 rounded disabled:opacity-50"
            >
              Next
            </button>
          </div>
        </>
      )}
    </div>
    </div>
    <Modal isOpen={isOpen} onClose={closeModal} className="max-w-[700px] m-4">
        <div className="no-scrollbar relative w-full max-w-[700px] overflow-y-auto rounded-3xl bg-white p-4 dark:bg-gray-900 lg:p-11">
          <div className="px-2 pr-14">
            <h4 className="mb-2 text-2xl font-semibold text-gray-800 dark:text-white/90">
              Add Personal Information
            </h4>
            {/* <p className="mb-6 text-sm text-gray-500 dark:text-gray-400 lg:mb-7">
              Update your details to keep your profile up-to-date.
            </p> */}
          </div>
          <form  className="flex flex-col">
            <div className="custom-scrollbar h-[450px] overflow-y-auto px-2 pb-3">
              <div>
                <h5 className="mb-5 text-lg font-medium text-gray-800 dark:text-white/90 lg:mb-6">
                
                </h5>

                <div className="grid grid-cols-1 gap-x-6 gap-y-5 lg:grid-cols-2">
                  <div className="col-span-2">
                    <Label>First Name</Label>
                    <Input
                      name="first_name"
                      type="text"
                      value={formData.first_name}
                      onChange={handleInputChange}
                      placeholder="First Name"
                    />
                  </div>

                  <div className="col-span-2">
                    <Label>Last Name</Label>
                    <Input 
                    name="last_name"
                    type="text" 
                    value={formData.last_name}
                    onChange={handleInputChange}
                    placeholder="Last Name"
                    />
                  </div>

                  <div className="col-span-2">
                    <Label>Email</Label>
                    <Input
                      name="email"
                      type="text"
                      value={formData.email}
                      onChange={handleInputChange}
                      placeholder="Email"
                    />
                  </div>

                  <div className="col-span-2">
                    <Label>Password</Label>
                    <Input 
                    name="password"
                    type="password"
                    value={formData.password}
                    onChange={handleInputChange}
                    placeholder="Password"
                    />
                  </div>
                </div>
              </div>
            </div>
            <div className="flex items-center gap-3 px-2 mt-6 lg:justify-end">
              <Button size="sm" variant="outline" onClick={closeModal}>
                Close
              </Button>
              <Button size="sm" onClick={handleAddUserSubmit}>
                Save Changes
              </Button>
            </div>
          </form>
        </div>
      </Modal>
    </>
  );
};

export default UserList;
